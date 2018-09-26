/*
Copyright (C)2013. OBIGO Inc. All rights reserved.
This software is covered by the license agreement between
the end user and OBIGO Inc. and may be
used and copied only in accordance with the terms of the
said agreement.
OBIGO Inc. assumes no responsibility or
liability for any errors or inaccuracies in this software,
or any consequential, incidental or indirect damage arising
out of the use of the software.
 */

#include <rapidjson/document.h>
#include <rapidjson/error/en.h>
#include <AVSCommon/Utils/JSON/JSONUtils.h>

#include "SampleApp/ConsolePrinter.h"
#include "AIDaemon/DirectiveHandler.h"
#include "AIDaemon/AIDaemon-IPC.h"

extern void sendMessagesIPC(std::string MethodID, rapidjson::Document *data);

namespace alexaClientSDK {
namespace sampleApp {

using namespace avsCommon::avs;
using namespace avsCommon::sdkInterfaces;
using namespace avsCommon::utils::json;

/// The namespace for this capability agent.
static const std::string NAMESPACE{"*"};

/// The name for RenderTemplate directive.
static const std::string NAME{"*"};

/// The RenderTemplate directive signature.
static const NamespaceAndName ALL{NAMESPACE, NAME};

DirectiveHandler::DirectiveHandler() {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
}

DirectiveHandler::~DirectiveHandler() {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
}

void DirectiveHandler::handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
    ConsolePrinter::simplePrint(directive->getUnparsedDirective());

    rapidjson::Document payload;
    payload.Parse(directive->getUnparsedDirective());
    sendMessagesIPC(AIDAEMON::METHODID_NOTI_DIRECTIVE, &payload);
}

void DirectiveHandler::preHandleDirective(std::shared_ptr<avsCommon::avs::AVSDirective> directive,
    std::unique_ptr<DirectiveHandlerResultInterface> result) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    ConsolePrinter::simplePrint(directive->getUnparsedDirective());
}

bool DirectiveHandler::handleDirective(const std::string& messageId) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
    return false;    
}
void DirectiveHandler::cancelDirective(const std::string& messageId) {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
}

void DirectiveHandler::onDeregistered() {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);
}

DirectiveHandlerConfiguration DirectiveHandler::getConfiguration() const {
    ConsolePrinter::simplePrint(__PRETTY_FUNCTION__);

    DirectiveHandlerConfiguration configuration;
    configuration[ALL] = BlockingPolicy::NON_BLOCKING;
    return configuration;    
}

}  // namespace sampleApp
}  // namespace alexaClientSDK
