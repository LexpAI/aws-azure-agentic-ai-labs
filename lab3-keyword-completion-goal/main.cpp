#include <functional>
#include <iostream>
#include <string>
#include <utility>
#include <variant>
#include <vector>

struct AgentState {
    std::string observation;
    int step_count = 0;
    bool done = false;
};

struct Goal {
    std::string description;
    std::function<bool(const AgentState&)> is_satisfied;
};

struct ToolCall {
    std::string name;
    std::string args;
};

struct Stop {
    std::string reason = "Agent decided to stop.";
};

using Action = std::variant<std::string, ToolCall, Stop>;

template <class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};

template <class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

std::string action_to_string(const Action& action) {
    return std::visit(
        overloaded{
            [](const std::string& text) {
                return "Answer(\"" + text + "\")";
            },
            [](const ToolCall& tool_call) {
                return "ToolCall(" + tool_call.name + ", \"" + tool_call.args + "\")";
            },
            [](const Stop& stop) {
                return "Stop(\"" + stop.reason + "\")";
            }},
        action);
}

struct TraceEntry {
    int step = 0;
    std::string action_desc;
    std::string new_observation;
};

struct RunMetrics {
    int total_steps = 0;
    int direct_answers = 0;
    int tool_calls = 0;
    int stop_actions = 0;
};

RunMetrics compute_metrics(const std::vector<TraceEntry>& trace) {
    RunMetrics metrics;
    metrics.total_steps = static_cast<int>(trace.size());

    for (const auto& entry : trace) {
        if (entry.action_desc.rfind("Answer(\"", 0) == 0) {
            metrics.direct_answers++;
        } else if (entry.action_desc.rfind("ToolCall(", 0) == 0) {
            metrics.tool_calls++;
        } else if (entry.action_desc.rfind("Stop(\"", 0) == 0) {
            metrics.stop_actions++;
        }
    }

    return metrics;
}

class SimpleAgent {
public:
    using ReasonFn = std::function<Action(const AgentState&)>;
    using ActFn = std::function<std::string(const Action&)>;

    SimpleAgent(ReasonFn reason, ActFn act)
        : reason_(std::move(reason)), act_(std::move(act)) {}

    TraceEntry step(AgentState& state) {
        Action action = reason_(state);
        std::string observation = act_(action);

        state.observation = observation;
        state.step_count++;

        if (std::holds_alternative<Stop>(action)) {
            state.done = true;
        }

        return {state.step_count, action_to_string(action), observation};
    }

    std::vector<TraceEntry> run(AgentState& state, const Goal& goal, int max_steps) {
        std::vector<TraceEntry> trace;

        while (!state.done && state.step_count < max_steps) {
            if (goal.is_satisfied(state)) {
                state.done = true;
                break;
            }

            trace.push_back(step(state));
        }

        return trace;
    }

private:
    ReasonFn reason_;
    ActFn act_;
};

void print_trace(
    const std::vector<TraceEntry>& trace,
    const std::string& goal_description,
    const AgentState& state,
    int max_steps) {
    std::cout << "\n-- Goal: " << goal_description << " --\n";

    for (const auto& entry : trace) {
        std::cout << "  Step " << entry.step << ": " << entry.action_desc << "\n";
        std::cout << "           -> obs: " << entry.new_observation << "\n";
    }

    const RunMetrics metrics = compute_metrics(trace);

    std::cout << "  Metrics: total_steps=" << metrics.total_steps
              << " direct_answers=" << metrics.direct_answers
              << " tool_calls=" << metrics.tool_calls
              << " stop_actions=" << metrics.stop_actions << "\n";

    if (!state.done && state.step_count >= max_steps) {
        std::cout << "  Safety guard triggered: max_steps="
                  << max_steps
                  << " reached before completion.\n";
    }
}

void run_keyword_workflow(
    const std::string& completion_keyword,
    int completion_step,
    int max_steps) {
    SimpleAgent agent(
        [completion_keyword, completion_step](const AgentState& state) -> Action {
            const int next_step = state.step_count + 1;

            if (next_step >= completion_step) {
                return completion_keyword + " - completed at step " + std::to_string(next_step);
            }

            return std::string("Working... step " + std::to_string(next_step));
        },
        [](const Action& action) -> std::string {
            if (std::holds_alternative<std::string>(action)) {
                return std::get<std::string>(action);
            }

            if (std::holds_alternative<Stop>(action)) {
                return "Stopped.";
            }

            return "Tool result.";
        });

    Goal keyword_goal{
        "Stop when observation contains the completion keyword",
        /*========== TODO_KEYWORD_GOAL 1/1 ==========*/

        // The policy emits COMPLETE, but this old predicate still watches DONE.
        // Replace this hard-coded check with completion_keyword.
        
        // Hint: Compare this lambda with the fixed goal shown in Task 5.
        
        [](const AgentState& state) {
            return state.observation.find("DONE") != std::string::npos;
        }

        /*========== END_OF_TODO_KEYWORD_GOAL ==========*/
    };

    AgentState state;
    state.observation = "Task not started.";

    const auto trace = agent.run(state, keyword_goal, max_steps);

    print_trace(trace, keyword_goal.description, state, max_steps);

    const bool completion_keyword_seen =
        state.observation.find(completion_keyword) != std::string::npos;
    const bool guard_triggered = !state.done && state.step_count >= max_steps;

    std::cout << "  Final obs: " << state.observation
              << " completion_keyword_seen=" << completion_keyword_seen
              << " guard_triggered=" << guard_triggered << "\n";
}

int main() {
    std::cout << "=== Keyword Completion Goal Lab ===\n";

    const std::string kCompletionKeyword = "COMPLETE";
    const int kCompletionStep = 4;
    const int kMaxStepsCap = 8;

    run_keyword_workflow(kCompletionKeyword, kCompletionStep, kMaxStepsCap);

    return 0;
}
