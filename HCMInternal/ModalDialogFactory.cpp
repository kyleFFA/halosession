#include "pch.h"
#include "ModalDialogFactory.h"
#include "FailedOptionalCheatServicesDialog.h"



	std::shared_ptr<IModalDialogVoid> ModalDialogFactory::makeFailedOptionalCheatServicesDialog
	(std::shared_ptr<GUIServiceInfo> guiFailures)
	{
		PLOG_VERBOSE << "factory creating FailedOptionalCheatServicesDialog";
		return std::make_unique<FailedOptionalCheatServicesDialog>(guiFailures);
	}
