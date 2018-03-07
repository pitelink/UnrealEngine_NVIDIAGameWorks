// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#include "K2Node_ControlRigEvaluator.h"
#include "EdGraphSchema_K2.h"
#include "K2Node_Event.h"
#include "K2Node_VariableGet.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_CallFunction.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_VariableSet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"
#include "BlueprintNodeSpawner.h"
#include "EditorCategoryUtils.h"
#include "BlueprintActionDatabaseRegistrar.h"
#include "ControlRig.h"
#include "Textures/SlateIcon.h"
#include "ControlRigBlueprintCompiler.h"
#include "ControlRigField.h"

#define LOCTEXT_NAMESPACE "K2Node_ControlRigEvaluator"

FString UK2Node_ControlRigEvaluator::ControlRigPinName(TEXT("ControlRig"));

UK2Node_ControlRigEvaluator::UK2Node_ControlRigEvaluator(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeTooltip = LOCTEXT("NodeTooltip", "Evaluates another ControlRig.");
}

void UK2Node_ControlRigEvaluator::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	struct GetMenuActions_Utils
	{
		static void SetNodeControlRigType(UEdGraphNode* NewNode, bool /*bIsTemplateNode*/, const UClass* ControlRigClass)
		{
			UK2Node_ControlRigEvaluator* EvaluatorNode = CastChecked<UK2Node_ControlRigEvaluator>(NewNode);
			EvaluatorNode->ControlRigType = const_cast<UClass*>(ControlRigClass);
		}

		static UBlueprintNodeSpawner* MakeControlRigEvaluatorAction(TSubclassOf<UEdGraphNode> const NodeClass, const UClass* ControlRigClass)
		{
			UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(NodeClass);
			check(NodeSpawner != nullptr);

			NodeSpawner->CustomizeNodeDelegate = UBlueprintNodeSpawner::FCustomizeNodeDelegate::CreateStatic(GetMenuActions_Utils::SetNodeControlRigType, ControlRigClass);
			NodeSpawner->DefaultMenuSignature.Category = const_cast<UClass*>(ControlRigClass)->GetDefaultObject<UControlRig>()->GetCategory();
			return NodeSpawner;
		}
	};

	if (const UObject* RegistrarTarget = ActionRegistrar.GetActionKeyFilter())
	{
		if (const UClass* RegistrarControlRigClass = Cast<UClass>(RegistrarTarget))
		{
			if (UBlueprintNodeSpawner* NodeSpawner = GetMenuActions_Utils::MakeControlRigEvaluatorAction(GetClass(), RegistrarControlRigClass))
			{
				ActionRegistrar.AddBlueprintAction(RegistrarControlRigClass, NodeSpawner);
			}
		}
	}
	else
	{
		UClass* NodeClass = GetClass();
		for (UClass* Class : TObjectRange<UClass>())
		{
			if (Class->IsChildOf(UControlRig::StaticClass()) && Class != UControlRig::StaticClass())
			{
				if (UBlueprintNodeSpawner* NodeSpawner = GetMenuActions_Utils::MakeControlRigEvaluatorAction(NodeClass, Class))
				{
					ActionRegistrar.AddBlueprintAction(Class, NodeSpawner);
				}
			}
		}
	}
}

FBlueprintNodeSignature UK2Node_ControlRigEvaluator::GetSignature() const
{
	FBlueprintNodeSignature NodeSignature = Super::GetSignature();

	if (ControlRigType.Get())
	{
		NodeSignature.AddSubObject(ControlRigType.Get());
	}

	return NodeSignature;
}

void UK2Node_ControlRigEvaluator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	if (PropertyChangedEvent.Property && PropertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(UK2Node_ControlRigEvaluator, ControlRigType))
	{
		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		Schema->ForceVisualizationCacheClear();

		DisabledInputs.Empty();
		DisabledOutputs.Empty();

		ReconstructNode();
	}
}

void UK2Node_ControlRigEvaluator::AllocateDefaultPins()
{
	Super::AllocateDefaultPins();

	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();

	TArray<TSharedRef<IControlRigField>> InputInfos = GetInputVariableInfo(DisabledInputs);
	TArray<TSharedRef<IControlRigField>> OutputInfos = GetOutputVariableInfo(DisabledOutputs);

	for (const TSharedRef<IControlRigField>& InputInfo : InputInfos)
	{
		UEdGraphPin* InputPin = CreatePin(EGPD_Input, InputInfo->GetPinType(), InputInfo->GetPinString());
		Schema->SetPinAutogeneratedDefaultValueBasedOnType(InputPin);
	}

	if (UClass* Class = GetControlRigClass())
	{
		CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Object, FString(), Class, ControlRigPinName, EPinContainerType::None, true);
	}

	for (const TSharedRef<IControlRigField>& OutputInfo : OutputInfos)
	{
		CreatePin(EGPD_Output, OutputInfo->GetPinType(), OutputInfo->GetPinString());
	}
}

FText UK2Node_ControlRigEvaluator::GetTooltipText() const
{
	if (UClass* Class = ControlRigType.Get())
	{
		if (UControlRig* ControlRig = Class->GetDefaultObject<UControlRig>())
		{
			return ControlRig->GetTooltipText();
		}
	}

	return LOCTEXT("EvaluateControlRigTooltip", "Evaluate ControlRig");
}

FText UK2Node_ControlRigEvaluator::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UClass* Class = ControlRigType.Get())
	{
		return Class->GetDisplayNameText();
	}

	return LOCTEXT("EvaluateControlRigTitle", "Evaluate ControlRig");
}

void UK2Node_ControlRigEvaluator::ExpandNode(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	if (CompilerContext.bIsFullCompile)
	{
		if (UClass* Class = ControlRigType.Get())
		{
			UEdGraphPin* DelegatePin = nullptr;
			UEdGraphPin* TempControlRigVariablePin = nullptr;
			ExpandInputs(CompilerContext, SourceGraph, DelegatePin, TempControlRigVariablePin);

			ExpandOutputs(CompilerContext, SourceGraph, DelegatePin, TempControlRigVariablePin);

			BreakAllNodeLinks();
		}
	}
}

void UK2Node_ControlRigEvaluator::ExpandInputs(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin*& OutDelegateOutputPin, UEdGraphPin*& OutTempControlRigVariablePin)
{
	if (UClass* Class = ControlRigType.Get())
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Hook up input parameter pins to setters
		TArray<UEdGraphPin*> InputPins;
		TArray<TSharedRef<IControlRigField>> FieldInfo;
		GetInputParameterPins(DisabledInputs, InputPins, FieldInfo);
		if (InputPins.Num() > 0)
		{
			// Create sub ControlRig
			int32 SubControlRigIndex = INDEX_NONE;
			UEdGraphPin* TempControlRigVariablePin = CreateAllocateSubControlRigNode(CompilerContext, SourceGraph, SubControlRigIndex);
			check(SubControlRigIndex != INDEX_NONE);

			// Add custom event to handle delegate
			UK2Node_Event* Event = CompilerContext.SpawnIntermediateEventNode<UK2Node_Event>(this, nullptr, SourceGraph);
			Event->EventReference.SetExternalDelegateMember(TEXT("PreEvaluateGatherInputs__DelegateSignature"));
			Event->CustomFunctionName = FBlueprintEditorUtils::FindUniqueKismetName(GetBlueprint(), FText::Format(LOCTEXT("DefaultPreEvaluateEventName", "ControlRigPreEvaluateEvent{0}"), FText::AsNumber(SubControlRigIndex)).ToString());
			Event->AllocateDefaultPins();
			UEdGraphPin* ExecPath = Event->FindPinChecked(Schema->PN_Then, EGPD_Output);

			for (int32 PinIndex = 0; PinIndex < InputPins.Num(); PinIndex++)
			{
				FieldInfo[PinIndex]->ExpandPin(GetControlRigClass(), CompilerContext, SourceGraph, this, InputPins[PinIndex], TempControlRigVariablePin, false, ExecPath);
			}

			OutDelegateOutputPin = Event->FindPinChecked(UK2Node_Event::DelegateOutputName);
			OutTempControlRigVariablePin = TempControlRigVariablePin;
		}
	}
}

UEdGraphPin* UK2Node_ControlRigEvaluator::CreateAllocateSubControlRigNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, int32& OutSubControlRigIndex)
{
	if (UClass* Class = ControlRigType.Get())
	{
		UK2Node_CallFunction* CallAllocateFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
		UFunction* GetHostFunction = FindFieldChecked<UFunction>(UControlRig::StaticClass(), GET_FUNCTION_NAME_CHECKED(UControlRig, GetOrAllocateSubControlRig));
		CallAllocateFunction->FunctionReference.SetFromField<UFunction>(GetHostFunction, true);
		CallAllocateFunction->bIsPureFunc = true;
		CallAllocateFunction->AllocateDefaultPins();

		FControlRigBlueprintCompilerContext& ControlRigCompilerContext = *static_cast<FControlRigBlueprintCompilerContext*>(&CompilerContext);
		OutSubControlRigIndex = ControlRigCompilerContext.GetNewControlRigAllocationIndex();
		UEdGraphPin* AllocationIndexPin = CallAllocateFunction->FindPinChecked(TEXT("AllocationIndex"));
		AllocationIndexPin->DefaultValue = LexicalConversion::ToString(OutSubControlRigIndex);

		UEdGraphPin* ControlRigClassPin = CallAllocateFunction->FindPinChecked(TEXT("ControlRigClass"));
		ControlRigClassPin->DefaultObject = Class;

		UEdGraphPin* ReturnValuePin = CallAllocateFunction->GetReturnValuePin();

		UK2Node_DynamicCast* DynamicCast = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
		DynamicCast->TargetType = Class;
		DynamicCast->SetPurity(true);
		DynamicCast->AllocateDefaultPins();

		CallAllocateFunction->GetReturnValuePin()->MakeLinkTo(DynamicCast->GetCastSourcePin());
		DynamicCast->NotifyPinConnectionListChanged(DynamicCast->GetCastSourcePin());

		return DynamicCast->GetCastResultPin();
	}

	return nullptr;
}

void UK2Node_ControlRigEvaluator::ExpandOutputs(class FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph, UEdGraphPin* PreEvaluateDelegatePin, UEdGraphPin* TempControlRigVariablePin)
{
	if (UClass* Class = ControlRigType.Get())
	{
		const UEdGraphSchema_K2* Schema = CompilerContext.GetSchema();

		// Create sub ControlRig if we haven't yet
		if (TempControlRigVariablePin == nullptr)
		{
			int32 PreEvaluateIndex;
			TempControlRigVariablePin = CreateAllocateSubControlRigNode(CompilerContext, SourceGraph, PreEvaluateIndex);
			check(TempControlRigVariablePin);
		}

		// Wire up ControlRig pin
		UEdGraphPin* ControlRigPin = FindPinChecked(ControlRigPinName, EGPD_Output);
		CompilerContext.MovePinLinksToIntermediate(*ControlRigPin, *TempControlRigVariablePin);

		// Call the evaluate function to populate our new ControlRig
		UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);

		// We need to use a different function signature for the no-delegate scenario as we dont support unbound delegates in blueprint right now.
		UFunction* EvaluateFunction; 
		if (PreEvaluateDelegatePin != nullptr)
		{
			EvaluateFunction = FindFieldChecked<UFunction>(UControlRig::StaticClass(), GET_FUNCTION_NAME_CHECKED(UControlRig, EvaluateControlRigWithInputs));
		}
		else
		{
			EvaluateFunction = FindFieldChecked<UFunction>(UControlRig::StaticClass(), GET_FUNCTION_NAME_CHECKED(UControlRig, EvaluateControlRig));
		}
		
		CallFunction->FunctionReference.SetFromField<UFunction>(EvaluateFunction, false);
		CallFunction->bIsPureFunc = true;
		CallFunction->AllocateDefaultPins();

		if (PreEvaluateDelegatePin != nullptr)
		{
			PreEvaluateDelegatePin->MakeLinkTo(CallFunction->FindPinChecked(TEXT("PreEvaluate")));
		}

		TempControlRigVariablePin->MakeLinkTo(CallFunction->FindPinChecked(TEXT("Target")));
		TempControlRigVariablePin = CallFunction->GetReturnValuePin();

		UK2Node_DynamicCast* DynamicCast = CompilerContext.SpawnIntermediateNode<UK2Node_DynamicCast>(this, SourceGraph);
		DynamicCast->TargetType = Class;
		DynamicCast->SetPurity(true);
		DynamicCast->AllocateDefaultPins();

		TempControlRigVariablePin->MakeLinkTo(DynamicCast->GetCastSourcePin());
		DynamicCast->NotifyPinConnectionListChanged(DynamicCast->GetCastSourcePin());
		TempControlRigVariablePin = DynamicCast->GetCastResultPin();

		TArray<UEdGraphPin*> OutputPins;
		TArray<TSharedRef<IControlRigField>> FieldInfo;
		GetOutputParameterPins(DisabledOutputs, OutputPins, FieldInfo);
		
		// Now expand pins according to the field type
		for (int32 PinIndex = 0; PinIndex < OutputPins.Num(); PinIndex++)
		{
			UEdGraphPin* ExecPath = nullptr;
			FieldInfo[PinIndex]->ExpandPin(GetControlRigClass(), CompilerContext, SourceGraph, this, OutputPins[PinIndex], TempControlRigVariablePin, false, ExecPath);
		}
	}
}

bool UK2Node_ControlRigEvaluator::IsCompatibleWithGraph(UEdGraph const* Graph) const
{
	UBlueprint* Blueprint = Cast<UBlueprint>(Graph->GetOuter());

	return Super::IsCompatibleWithGraph(Graph) && Blueprint && Blueprint->ParentClass && Blueprint->ParentClass->IsChildOf(UControlRig::StaticClass());
}

const UClass* UK2Node_ControlRigEvaluator::GetControlRigClassImpl() const
{
	return ControlRigType.Get();
}

#undef LOCTEXT_NAMESPACE