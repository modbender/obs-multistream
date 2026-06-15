#include "MultistreamOutput.hpp"

#include <widgets/OBSBasic.hpp>
#include <utility/BasicOutputHandler.hpp>
#include <utility/CanvasManager.hpp>
#include <utility/OutputBinding.hpp>
#include <utility/StreamProfileManager.hpp>

#include <QMetaObject>

MultistreamOutput::MultistreamOutput(OBSBasic *main_) : main(main_) {}

MultistreamOutput::~MultistreamOutput()
{
	StopAll();
}

video_t *MultistreamOutput::VideoForCanvas(const std::string &canvasUuid)
{
	const CanvasDefinition &def = main->GetCanvasManager().Default();
	if (canvasUuid == def.uuid) {
		return obs_get_video();
	}
	for (const OBS::Canvas &canvas : main->GetCanvases()) {
		if (canvasUuid == obs_canvas_get_uuid(canvas)) {
			return obs_canvas_get_video(static_cast<obs_canvas_t *>(canvas));
		}
	}
	return nullptr;
}

MultistreamOutput::CanvasEncoders *MultistreamOutput::EnsureCanvasEncoders(const std::string &canvasUuid)
{
	for (CanvasEncoders &ce : canvasEncoders) {
		if (ce.canvasUuid == canvasUuid && ce.video && ce.audio) {
			return &ce;
		}
	}

	const CanvasDefinition &def = main->GetCanvasManager().Default();
	const CanvasDefinition *cdef = (canvasUuid == def.uuid) ? &def : main->GetCanvasManager().Find(canvasUuid);
	if (!cdef) {
		return nullptr;
	}
	video_t *video = VideoForCanvas(canvasUuid);
	if (!video) {
		blog(LOG_WARNING, "Multistream: no video mix for canvas %s", canvasUuid.c_str());
		return nullptr;
	}

	/* Resolve encoder ids/settings, honoring 'use default' inheritance from the
	 * Default canvas. If the id is empty or marked use-default, fall back to the
	 * Default canvas encoder. */
	const CanvasEncoderDef &vdef = (cdef->video.useDefault || cdef->video.id.empty()) ? def.video : cdef->video;
	const CanvasEncoderDef &adef = (cdef->audio.useDefault || cdef->audio.id.empty()) ? def.audio : cdef->audio;
	if (vdef.id.empty() || adef.id.empty()) {
		blog(LOG_WARNING, "Multistream: canvas %s has no usable encoders", canvasUuid.c_str());
		return nullptr;
	}

	CanvasEncoders ce;
	ce.canvasUuid = canvasUuid;
	std::string vname = "multistream_v_" + canvasUuid;
	std::string aname = "multistream_a_" + canvasUuid;
	ce.video =
		OBSEncoderAutoRelease(obs_video_encoder_create(vdef.id.c_str(), vname.c_str(), vdef.settings, nullptr));
	ce.audio = OBSEncoderAutoRelease(
		obs_audio_encoder_create(adef.id.c_str(), aname.c_str(), adef.settings, 0, nullptr));
	if (!ce.video || !ce.audio) {
		return nullptr;
	}
	obs_encoder_set_video(ce.video, video);
	obs_encoder_set_audio(ce.audio, obs_get_audio());

	canvasEncoders.push_back(std::move(ce));
	return &canvasEncoders.back();
}

bool MultistreamOutput::ProfileLiveElsewhere(const std::string &bindingUuid, const std::string &profileUuid) const
{
	if (profileUuid.empty()) {
		return false;
	}
	for (const auto &lo : live) {
		if (lo->bindingUuid != bindingUuid && lo->profileUuid == profileUuid) {
			return true;
		}
	}
	return false;
}

bool MultistreamOutput::StartOutput(const std::string &bindingUuid)
{
	if (IsLive(bindingUuid)) {
		return true;
	}
	OutputBinding *b = main->GetOutputBindings().Find(bindingUuid);
	if (!b || b->profileUuid.empty()) {
		return false;
	}
	if (ProfileLiveElsewhere(bindingUuid, b->profileUuid)) {
		blog(LOG_WARNING, "Multistream: profile already live; refusing second output");
		return false;
	}
	StreamProfile *p = main->GetStreamProfileManager().Find(b->profileUuid);
	if (!p) {
		return false;
	}
	CanvasEncoders *ce = EnsureCanvasEncoders(b->canvasUuid);
	if (!ce) {
		return false;
	}

	auto lo = std::make_unique<LiveOutput>();
	lo->bindingUuid = bindingUuid;
	lo->profileUuid = b->profileUuid;
	lo->canvasUuid = b->canvasUuid;

	std::string sname = "multistream_svc_" + bindingUuid;
	lo->service =
		OBSServiceAutoRelease(obs_service_create(p->serviceId.c_str(), sname.c_str(), p->settings, nullptr));
	if (!lo->service) {
		return false;
	}

	const char *type = GetStreamOutputType(lo->service);
	if (!type) {
		type = "rtmp_output";
	}
	std::string oname = "multistream_out_" + bindingUuid;
	lo->output = OBSOutputAutoRelease(obs_output_create(type, oname.c_str(), nullptr, nullptr));
	if (!lo->output) {
		return false;
	}

	lo->startSignal.Connect(obs_output_get_signal_handler(lo->output), "start", OnOutputStart, this);
	lo->stopSignal.Connect(obs_output_get_signal_handler(lo->output), "stop", OnOutputStop, this);

	obs_output_set_video_encoder(lo->output, ce->video);
	obs_output_set_audio_encoder(lo->output, ce->audio, 0);
	obs_output_set_service(lo->output, lo->service);

	lo->state = State::Connecting;
	LiveOutput *raw = lo.get();
	live.push_back(std::move(lo));

	if (!obs_output_start(raw->output)) {
		const char *err = obs_output_get_last_error(raw->output);
		raw->lastError = err ? err : "";
		raw->state = State::Error;
		blog(LOG_WARNING, "Multistream: output '%s' failed to start: %s", oname.c_str(),
		     raw->lastError.c_str());
		NotifyChanged();
		return false;
	}
	NotifyChanged();
	return true;
}

void MultistreamOutput::StopOutput(const std::string &bindingUuid)
{
	LiveOutput *lo = FindLive(bindingUuid);
	if (!lo) {
		return;
	}
	if (lo->output) {
		obs_output_stop(lo->output);
	}
	RemoveLive(bindingUuid);
	NotifyChanged();
}

int MultistreamOutput::StartAll()
{
	int started = 0;
	for (const OutputBinding &b : main->GetOutputBindings().bindings) {
		if (b.enabled && StartOutput(b.uuid)) {
			started++;
		}
	}
	return started;
}

void MultistreamOutput::StopAll()
{
	for (auto &lo : live) {
		if (lo->output) {
			obs_output_stop(lo->output);
		}
	}
	live.clear();
	/* Encoders are released when the handler is destroyed or rebuilt; keep the
	 * cache so a quick restart reuses them, but they hold no output refs now. */
	NotifyChanged();
}

bool MultistreamOutput::AnyActive() const
{
	return !live.empty();
}

bool MultistreamOutput::IsLive(const std::string &bindingUuid) const
{
	for (const auto &lo : live) {
		if (lo->bindingUuid == bindingUuid) {
			return true;
		}
	}
	return false;
}

MultistreamOutput::LiveOutput *MultistreamOutput::FindLive(const std::string &bindingUuid)
{
	for (auto &lo : live) {
		if (lo->bindingUuid == bindingUuid) {
			return lo.get();
		}
	}
	return nullptr;
}

void MultistreamOutput::RemoveLive(const std::string &bindingUuid)
{
	for (auto it = live.begin(); it != live.end(); ++it) {
		if ((*it)->bindingUuid == bindingUuid) {
			live.erase(it);
			return;
		}
	}
}

std::vector<MultistreamOutput::OutputStatus> MultistreamOutput::Statuses() const
{
	std::vector<OutputStatus> out;
	for (const OutputBinding &b : main->GetOutputBindings().bindings) {
		if (!b.enabled) {
			continue;
		}
		OutputStatus st;
		st.bindingUuid = b.uuid;
		st.canvasUuid = b.canvasUuid;
		if (!b.profileUuid.empty()) {
			if (StreamProfile *p = main->GetStreamProfileManager().Find(b.profileUuid)) {
				st.profileLabel = p->DisplayName();
			}
		}
		for (const auto &lo : live) {
			if (lo->bindingUuid == b.uuid) {
				st.state = lo->state;
				st.lastError = lo->lastError;
				break;
			}
		}
		out.push_back(std::move(st));
	}
	return out;
}

void MultistreamOutput::NotifyChanged()
{
	if (onStatusChanged) {
		QMetaObject::invokeMethod(main, [cb = onStatusChanged] { cb(); }, Qt::QueuedConnection);
	}
}

void MultistreamOutput::OnOutputStart(void *data, calldata_t *cd)
{
	auto self = static_cast<MultistreamOutput *>(data);
	obs_output_t *out = (obs_output_t *)calldata_ptr(cd, "output");
	for (auto &lo : self->live) {
		if (lo->output == out) {
			lo->state = State::Live;
			break;
		}
	}
	self->NotifyChanged();
}

void MultistreamOutput::OnOutputStop(void *data, calldata_t *cd)
{
	auto self = static_cast<MultistreamOutput *>(data);
	obs_output_t *out = (obs_output_t *)calldata_ptr(cd, "output");
	for (auto &lo : self->live) {
		if (lo->output == out) {
			lo->state = State::Idle;
			break;
		}
	}
	self->NotifyChanged();
}
