// Copyright 1998-2017 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GraphEditAction.h"
#include "Layout/SlateRect.h"
#include "EdGraphSchema_K2.h"
#include "ComponentInstanceDataCache.h"
#include "GameFramework/DamageType.h"
#include "Components/ActorComponent.h"
#include "EdGraphSchema_K2_Actions.h"

#include "Styling/SlateBrush.h"
#include "BlueprintNodeSignature.h"
#include "K2Node_ActorBoundEvent.h"
#include "K2Node_AddComponent.h"
#include "K2Node_AddDelegate.h"
#include "K2Node_AssignmentStatement.h"
#include "K2Node_BaseAsyncTask.h"
#include "K2Node_BreakStruct.h"
#include "K2Node_CallArrayFunction.h"
#include "K2Node_CallDataTableFunction.h"
#include "K2Node_CallDelegate.h"
#include "K2Node_CallFunctionOnMember.h"
#include "K2Node_CallMaterialParameterCollectionFunction.h"
#include "K2Node_CallParentFunction.h"
#include "K2Node_ClearDelegate.h"
#include "K2Node_CommutativeAssociativeBinaryOperator.h"
#include "K2Node_ConstructObjectFromClass.h"
#include "K2Node_DoOnceMultiInput.h"
#include "K2Node_ComponentBoundEvent.h"
#include "K2Node_Copy.h"
#include "K2Node_CreateDelegate.h"
#include "K2Node_CustomEvent.h"
#include "K2Node_DelegateSet.h"
#include "K2Node_DynamicCast.h"
#include "K2Node_EaseFunction.h"
#include "K2Node_ExecutionSequence.h"
#include "K2Node_FormatText.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_FunctionResult.h"
#include "K2Node_GetClassDefaults.h"
#include "Engine/DataTable.h"
#include "K2Node_GetDataTableRow.h"
#include "K2Node_GetArrayItem.h"
#include "K2Node_IfThenElse.h"
#include "K2Node_InputAction.h"
#include "K2Node_InputAxisEvent.h"
#include "K2Node_InputKey.h"
#include "K2Node_InputTouch.h"
#include "K2Node_Knot.h"
#include "K2Node_Literal.h"
#include "K2Node_MacroInstance.h"
#include "K2Node_MakeArray.h"
#include "K2Node_MathExpression.h"
#include "K2Node_MatineeController.h"
#include "K2Node_RemoveDelegate.h"
#include "K2Node_Select.h"
#include "K2Node_Self.h"
#include "K2Node_SpawnActor.h"
#include "K2Node_SpawnActorFromClass.h"
#include "K2Node_SwitchInteger.h"
#include "K2Node_SwitchName.h"
#include "K2Node_TemporaryVariable.h"
#include "K2Node_Timeline.h"
#include "K2Node_VariableGet.h"
#include "K2Node_VariableSet.h"
#include "K2Node_SetFieldsInStruct.h"
#include "K2Node_TunnelBoundary.h"
