#pragma once

#include "OutputBinding.hpp"

#include <string>

// Holds the OutputBindings (profile x canvas routing edges) for the new frontend
// and round-trips them to a standalone output_bindings.json under the shared
// <config>/obs-multistream/basic directory.
//
// NOTE: the legacy frontend embeds these inside each scene-collection JSON (an
// "output_bindings" array), so bindings are per scene-collection there. Until
// scene-collection save/load is wired into the new frontend (4.4.1+), we own a
// single standalone file. The wire format is the SAME array the legacy
// OutputBindings::ToDataArray/FromDataArray produce, so the per-collection
// migration later is purely about WHERE the array lives, not its shape.
class OutputBindingStore {
public:
	void Load(); // read output_bindings.json into bindings (empty if absent)
	void Save() const;

	OutputBindings &Bindings() { return bindings; }
	const OutputBindings &Bindings() const { return bindings; }

	// Drop all bindings for teardown leak measurement.
	void Clear() { bindings.bindings.clear(); }

	static std::string FilePath(); // <config>/obs-multistream/basic/output_bindings.json

private:
	OutputBindings bindings;
};
