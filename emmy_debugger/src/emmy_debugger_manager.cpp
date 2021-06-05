#include "emmy_debugger/emmy_debugger_manager.h"

EmmyDebuggerManager::EmmyDebuggerManager()
	: stateBreak(std::make_shared<HookStateBreak>()),
	  stateContinue(std::make_shared<HookStateContinue>()),
	  stateStepOver(std::make_shared<HookStateStepOver>()),
	  stateStop(std::make_shared<HookStateStop>()),
	  stateStepIn(std::make_shared<HookStateStepIn>()),
	  stateStepOut(std::make_shared<HookStateStepOut>())
{
}

EmmyDebuggerManager::~EmmyDebuggerManager()
{
}

std::shared_ptr<Debugger> EmmyDebuggerManager::GetDebugger(lua_State* L)
{
	std::lock_guard<std::mutex> lock(debuggerMtx);
	auto it = debuggers.find(L);
	if (it != debuggers.end())
	{
		return it->second;
	}
	else
	{
		return nullptr;
	}
}

std::shared_ptr<Debugger> EmmyDebuggerManager::AddDebugger(lua_State* L)
{
	std::lock_guard<std::mutex> lock(debuggerMtx);
	auto it = debuggers.find(L);
	if (it != debuggers.end())
	{
		return it->second;
	}
	else
	{
		auto debugger = std::make_shared<Debugger>(L, shared_from_this());
		debuggers.insert({L, debugger});
		return debugger;
	}
}

std::shared_ptr<Debugger> EmmyDebuggerManager::RemoveDebugger(lua_State* L)
{
	std::lock_guard<std::mutex> lock(debuggerMtx);
	auto it = debuggers.find(L);
	if (it != debuggers.end())
	{
		auto debugger = it->second;
		debuggers.erase(it);
		return debugger;
	}
	return nullptr;
}

std::vector<std::shared_ptr<Debugger>> EmmyDebuggerManager::GetDebuggers()
{
	std::lock_guard<std::mutex> lock(debuggerMtx);
	std::vector<std::shared_ptr<Debugger>> debuggerVector;
	for (auto it : debuggers)
	{
		debuggerVector.push_back(it.second);
	}
	return debuggerVector;
}

std::shared_ptr<Debugger> EmmyDebuggerManager::GetBreakedpoint()
{
	std::lock_guard<std::mutex> lock(breakDebuggerMtx);
	return breakedDebugger;
}

void EmmyDebuggerManager::SetBreakedpoint(std::shared_ptr<Debugger> debugger)
{
	std::lock_guard<std::mutex> lock(breakDebuggerMtx);
	breakedDebugger = debugger;
}

bool EmmyDebuggerManager::IsDebuggerEmpty()
{
	std::lock_guard<std::mutex> lock(debuggerMtx);
	return debuggers.empty();
}

void EmmyDebuggerManager::AddBreakpoint(std::shared_ptr<BreakPoint> breakpoint)
{
	std::lock_guard<std::mutex> lock(breakpointsMtx);
	breakpoints.push_back(breakpoint);
	RefreshLineSet();
}

std::vector<std::shared_ptr<BreakPoint>> EmmyDebuggerManager::GetBreakpoints()
{
	std::lock_guard<std::mutex> lock(breakpointsMtx);
	return breakpoints;
}

void EmmyDebuggerManager::RemoveBreakpoint(const std::string& file, int line)
{
	// �������Ƚϴ�
	std::lock_guard<std::mutex> lock(breakpointsMtx);
	std::string lowerCaseFile = file;
	std::transform(file.begin(), file.end(), lowerCaseFile.begin(), ::tolower);
	auto it = breakpoints.begin();
	while (it != breakpoints.end())
	{
		const auto bp = *it;
		if (bp->file == lowerCaseFile && bp->line == line)
		{
			breakpoints.erase(it);
			break;
		}
		++it;
	}
	RefreshLineSet();
}

void EmmyDebuggerManager::RemoveAllBreakPoints()
{
	std::lock_guard<std::mutex> lock(breakpointsMtx);
	breakpoints.clear();
	lineSet.clear();
}

void EmmyDebuggerManager::RefreshLineSet()
{
	lineSet.clear();
	for (auto bp : breakpoints)
	{
		lineSet.insert(bp->line);
	}
}

std::set<int> EmmyDebuggerManager::GetLineSet()
{
	std::lock_guard<std::mutex> lock(breakpointsMtx);
	return lineSet;
}

void EmmyDebuggerManager::HandleBreak(lua_State* L)
{
	auto debugger = GetDebugger(L);

	if (!debugger)
	{
		debugger = AddDebugger(L);
	}

	SetBreakedpoint(debugger);

	debugger->HandleBreak();
}

void EmmyDebuggerManager::DoAction(DebugAction action)
{
	auto debugger = GetBreakedpoint();
	if (debugger)
	{
		debugger->DoAction(action);
	}
}

void EmmyDebuggerManager::Eval(std::shared_ptr<EvalContext> ctx)
{
	auto debugger = GetBreakedpoint();
	if (debugger)
	{
		debugger->Eval(ctx, false);
	}
}
