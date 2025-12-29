/**
 * File: behaviorTwentyQuestions.cpp
 * Description: Simple 20 Questions game inspired by the blackjack behavior.
 */

#include "engine/aiComponent/behaviorComponent/behaviors/twentyQuestions/behaviorTwentyQuestions.h"

#include "engine/aiComponent/behaviorComponent/behaviorContainer.h"
#include "engine/aiComponent/behaviorComponent/behaviors/animationWrappers/behaviorTextToSpeechLoop.h"
#include "engine/aiComponent/behaviorComponent/userIntentComponent.h"
#include "engine/components/localeComponent.h"
#include "clad/cloud/mic.h"

#include <array>

namespace Anki {
namespace Vector {

namespace {
  static const UserIntentTag affirmativeIntent = USER_INTENT(imperative_affirmative);
  static const UserIntentTag negativeIntent = USER_INTENT(imperative_negative);
  static const UserIntentTag silenceIntent = USER_INTENT(silence);
  static const UserIntentTag unmatchedIntent = USER_INTENT(unmatched_intent);

  const char* kIntroKey = "TwentyQuestions.Intro";
  const char* kOutroKey = "TwentyQuestions.Outro";
  const char* kUnknownResponseKey = "TwentyQuestions.Unknown";

  const std::array<const char*, 20> kQuestionKeys = {
    "TwentyQuestions.Question01",
    "TwentyQuestions.Question02",
    "TwentyQuestions.Question03",
    "TwentyQuestions.Question04",
    "TwentyQuestions.Question05",
    "TwentyQuestions.Question06",
    "TwentyQuestions.Question07",
    "TwentyQuestions.Question08",
    "TwentyQuestions.Question09",
    "TwentyQuestions.Question10",
    "TwentyQuestions.Question11",
    "TwentyQuestions.Question12",
    "TwentyQuestions.Question13",
    "TwentyQuestions.Question14",
    "TwentyQuestions.Question15",
    "TwentyQuestions.Question16",
    "TwentyQuestions.Question17",
    "TwentyQuestions.Question18",
    "TwentyQuestions.Question19",
    "TwentyQuestions.Question20"
  };
}

BehaviorTwentyQuestions::InstanceConfig::InstanceConfig()
{
}

BehaviorTwentyQuestions::DynamicVariables::DynamicVariables()
: state(EState::Intro)
, questionIndex(0)
, unknownResponses(0)
{
}

BehaviorTwentyQuestions::BehaviorTwentyQuestions(const Json::Value& config)
: ICozmoBehavior(config)
{
}

BehaviorTwentyQuestions::~BehaviorTwentyQuestions()
{
}

bool BehaviorTwentyQuestions::WantsToBeActivatedBehavior() const
{
  return true;
}

void BehaviorTwentyQuestions::GetAllDelegates(std::set<IBehavior*>& delegates) const
{
  delegates.insert(_iConfig.ttsBehavior.get());
}

void BehaviorTwentyQuestions::GetBehaviorOperationModifiers(BehaviorOperationModifiers& modifiers) const
{
  modifiers.wantsToBeActivatedWhenOffTreads = true;
  modifiers.wantsToBeActivatedWhenOnCharger = true;
  modifiers.behaviorAlwaysDelegates = false;
}

void BehaviorTwentyQuestions::InitBehavior()
{
  const auto& BC = GetBEI().GetBehaviorContainer();
  BC.FindBehaviorByIDAndDowncast(BEHAVIOR_ID(TwentyQuestionsTextToSpeech),
                                 BEHAVIOR_CLASS(TextToSpeechLoop),
                                 _iConfig.ttsBehavior);
}

void BehaviorTwentyQuestions::OnBehaviorActivated()
{
  _dVars = DynamicVariables();
  PopulateQuestions();

  if (!_iConfig.ttsBehavior) {
    CancelSelf();
    return;
  }

  _iConfig.ttsBehavior->SetTextToSay(GetLocalizedString(kIntroKey));
  if (_iConfig.ttsBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_iConfig.ttsBehavior.get(), [this]() {
      _dVars.state = DynamicVariables::EState::Asking;
      AskCurrentQuestion();
    });
  }
}

void BehaviorTwentyQuestions::OnBehaviorDeactivated()
{
  _questions.clear();
}

void BehaviorTwentyQuestions::BehaviorUpdate()
{
  if (!IsActivated()) {
    return;
  }

  if (_dVars.state == DynamicVariables::EState::WaitingForResponse) {
    if (HandlePendingIntent()) {
      AskCurrentQuestion();
    }
  }
}

void BehaviorTwentyQuestions::PopulateQuestions()
{
  _questions.clear();
  _questions.reserve(kQuestionKeys.size());
  for (const auto& key : kQuestionKeys) {
    _questions.push_back(GetLocalizedString(key));
  }
}

void BehaviorTwentyQuestions::AskCurrentQuestion()
{
  if (_dVars.questionIndex >= _questions.size()) {
    FinishGame();
    return;
  }

  if (!_iConfig.ttsBehavior) {
    CancelSelf();
    return;
  }

  _dVars.state = DynamicVariables::EState::Asking;
  _iConfig.ttsBehavior->SetTextToSay(_questions[_dVars.questionIndex]);
  if (_iConfig.ttsBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_iConfig.ttsBehavior.get(), [this]() {
      StartListeningForResponse();
    });
  }
}

void BehaviorTwentyQuestions::StartListeningForResponse()
{
  _dVars.state = DynamicVariables::EState::WaitingForResponse;
  GetBehaviorComp<UserIntentComponent>().StartWakeWordlessStreaming(CloudMic::StreamType::Normal);
}

bool BehaviorTwentyQuestions::HandlePendingIntent()
{
  UserIntentComponent& uic = GetBehaviorComp<UserIntentComponent>();

  if (uic.IsUserIntentPending(affirmativeIntent)) {
    uic.DropUserIntent(affirmativeIntent);
    ++_dVars.questionIndex;
    return true;
  }

  if (uic.IsUserIntentPending(negativeIntent)) {
    uic.DropUserIntent(negativeIntent);
    ++_dVars.questionIndex;
    return true;
  }

  if (uic.IsUserIntentPending(silenceIntent) || uic.IsUserIntentPending(unmatchedIntent)) {
    if (uic.IsUserIntentPending(silenceIntent)) {
      uic.DropUserIntent(silenceIntent);
    }
    if (uic.IsUserIntentPending(unmatchedIntent)) {
      uic.DropUserIntent(unmatchedIntent);
    }

    ++_dVars.unknownResponses;

    if (_iConfig.ttsBehavior) {
      _iConfig.ttsBehavior->SetTextToSay(GetLocalizedString(kUnknownResponseKey));
      if (_iConfig.ttsBehavior->WantsToBeActivated()) {
        DelegateIfInControl(_iConfig.ttsBehavior.get());
      }
    }

    ++_dVars.questionIndex;
    return true;
  }

  return false;
}

void BehaviorTwentyQuestions::FinishGame()
{
  _dVars.state = DynamicVariables::EState::Concluding;

  if (!_iConfig.ttsBehavior) {
    CancelSelf();
    return;
  }

  _iConfig.ttsBehavior->SetTextToSay(GetLocalizedString(kOutroKey));
  if (_iConfig.ttsBehavior->WantsToBeActivated()) {
    DelegateIfInControl(_iConfig.ttsBehavior.get(), [this]() {
      CancelSelf();
    });
  }
}

std::string BehaviorTwentyQuestions::GetLocalizedString(const std::string& key) const
{
  const auto& localeComponent = GetBEI().GetRobotInfo().GetLocaleComponent();
  return localeComponent.GetString(key);
}

} // namespace Vector
} // namespace Anki
