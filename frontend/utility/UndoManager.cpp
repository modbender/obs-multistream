#include "UndoManager.hpp"

#include <utility>

void UndoManager::AddAction(const std::string &name, const Cb &undo, const Cb &redo, const std::string &undoData,
			    const std::string &redoData)
{
	// Don't record sub-actions while a mutation (or an in-flight undo/redo) has
	// suppressed recording.
	if (!IsEnabled()) {
		return;
	}

	undoItems.push_back(Item{name, undoData, redoData, undo, redo});
	// A fresh action invalidates the redo branch.
	redoItems.clear();
	notify();
}

void UndoManager::Undo()
{
	if (undoItems.empty()) {
		return;
	}

	Item item = std::move(undoItems.back());
	undoItems.pop_back();

	// Re-applying a snapshot must not record a new action.
	PushDisabled();
	if (item.undo) {
		item.undo(item.undoData);
	}
	PopDisabled();

	redoItems.push_back(std::move(item));
	notify();
}

void UndoManager::Redo()
{
	if (redoItems.empty()) {
		return;
	}

	Item item = std::move(redoItems.back());
	redoItems.pop_back();

	// Re-applying a snapshot must not record a new action.
	PushDisabled();
	if (item.redo) {
		item.redo(item.redoData);
	}
	PopDisabled();

	undoItems.push_back(std::move(item));
	notify();
}

void UndoManager::Clear()
{
	undoItems.clear();
	redoItems.clear();
	notify();
}

void UndoManager::PushDisabled()
{
	disableRefs++;
}

void UndoManager::PopDisabled()
{
	if (disableRefs > 0) {
		disableRefs--;
	}
}

UndoManager::State UndoManager::GetState() const
{
	State s;
	s.canUndo = !undoItems.empty();
	s.canRedo = !redoItems.empty();
	s.undoName = undoItems.empty() ? std::string() : undoItems.back().name;
	s.redoName = redoItems.empty() ? std::string() : redoItems.back().name;
	return s;
}
