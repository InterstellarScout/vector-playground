/**
 * File: behaviorTwentyQuestions.h
 * Description: Simple 20 Questions game inspired by the blackjack behavior.
 */

#ifndef __Engine_AiComponent_BehaviorComponent_Behaviors_TwentyQuestions_BehaviorTwentyQuestions_H__
#define __Engine_AiComponent_BehaviorComponent_Behaviors_TwentyQuestions_BehaviorTwentyQuestions_H__

#include "engine/aiComponent/behaviorComponent/iCozmoBehavior.h"

#include <memory>
#include <string>
#include <vector>

namespace Anki {
namespace Vector {

class TextToSpeechLoop;

class BehaviorTwentyQuestions : public ICozmoBehavior
{
public:
  struct InstanceConfig
  {
    InstanceConfig();

    std::shared_ptr<TextToSpeechLoop> ttsBehavior;
  };

  struct DynamicVariables
  {
    DynamicVariables();

    enum class EState
    {
      Intro,
      Asking,
      WaitingForResponse,
      Concluding
    };

    EState state;
    size_t questionIndex;
    size_t unknownResponses;
  };

  explicit BehaviorTwentyQuestions(const Json::Value& config);
  ~BehaviorTwentyQuestions() override;

  bool WantsToBeActivatedBehavior() const override;
  void GetAllDelegates(std::set<IBehavior*>& delegates) const override;
  void GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const override;
  void InitBehavior() override;
  void OnBehaviorActivated() override;
  void BehaviorUpdate() override;
  void OnBehaviorDeactivated() override;

private:
  void PopulateQuestions();
  void AskCurrentQuestion();
  void StartListeningForResponse();
  bool HandlePendingIntent();
  void FinishGame();
  std::string GetLocalizedString(const std::string& key) const;

  InstanceConfig _iConfig;
  DynamicVariables _dVars;
  std::vector<std::string> _questions;
};

} // namespace Vector
} // namespace Anki

#endif // __Engine_AiComponent_BehaviorComponent_Behaviors_TwentyQuestions_BehaviorTwentyQuestions_H__
