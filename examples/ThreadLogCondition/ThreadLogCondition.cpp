#include <cstddef>
#include <iterator>

#include <Logme/Logme.h>

// LoggerCondition()'s member-override trick (see examples/LoggerConditionOverride)
// only helps code that has an object of its own to cache a per-request
// answer on. Some code doesn't: a parsed rule tree shared across many
// different requests has no per-request state to attach a LoggerCondition()
// member to. LogmeThreadCondition lets that code declare, once, right
// before it starts a stretch of work on the current thread, that every
// log call made until the stretch ends can be skipped -- exactly like
// LogmeThreadSubsystem declares which subsystem those calls report under,
// just for the LoggerCondition() gate instead.

static Logme::ChannelPtr EnsureVisibleChannel(const Logme::ID& id)
{
  auto ch = Logme::Instance->CreateChannel(id);
  ch->AddLink(::CH);
  return ch;
}

// Stands in for a shared, parsed rule tree: the same RuleNode instances are
// walked on behalf of every request, so they have no per-request member to
// cache a LoggerCondition() override on.
struct RuleNode
{
  const char* Name;

  void Evaluate(const Logme::ChannelPtr& ch) const
  {
    LogmeI(ch, "evaluating rule: %s", Name);
  }
};

static void EvaluateRules(const Logme::ChannelPtr& ch, const RuleNode* rules, size_t count)
{
  for (size_t i = 0; i < count; ++i)
    rules[i].Evaluate(ch);
}

int main()
{
  auto ch = EnsureVisibleChannel(Logme::ID{"thread_log_condition_example"});

  RuleNode rules[] = { {"is_admin"}, {"host_allowed"}, {"rate_under_limit"} };

  // A request whose own logging was already decided to be off: gate the
  // whole rule-tree walk with one thread-local check instead of adding
  // per-request state to every RuleNode.
  {
    LogmeThreadCondition(false);
    EvaluateRules(ch, rules, std::size(rules)); // suppressed: none of these reach DoLog()
  }

  // A request whose logging is on: same rule tree, same code path, nothing
  // to opt back into -- clearing the scope (or setting true) is enough.
  {
    LogmeThreadCondition(true);
    EvaluateRules(ch, rules, std::size(rules));
  }

  // Outside any LogmeThreadCondition scope, behaves exactly as before this
  // existed.
  EvaluateRules(ch, rules, std::size(rules));

  return 0;
}
