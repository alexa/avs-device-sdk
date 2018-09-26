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

#ifndef ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_DIRECTIVE_HANDLER_H_
#define ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_DIRECTIVE_HANDLER_H_

#include <AVSCommon/SDKInterfaces/DirectiveHandlerInterface.h>
#include "AVSCommon/SDKInterfaces/DirectiveHandlerResultInterface.h"

namespace alexaClientSDK {
namespace sampleApp {

class DirectiveHandler : public avsCommon::sdkInterfaces::DirectiveHandlerInterface {

public:
    DirectiveHandler();
    virtual ~DirectiveHandler();
    void handleDirectiveImmediately(std::shared_ptr<avsCommon::avs::AVSDirective> directive) override;
    void preHandleDirective(
        std::shared_ptr<avsCommon::avs::AVSDirective> directive,
        std::unique_ptr<avsCommon::sdkInterfaces::DirectiveHandlerResultInterface> result) override;
    bool handleDirective(const std::string& messageId) override;
    void cancelDirective(const std::string& messageId) override;
    void onDeregistered() override;
    avsCommon::avs::DirectiveHandlerConfiguration getConfiguration() const ;
};

}  // namespace sampleApp
}  // namespace alexaClientSDK

#endif  // ALEXA_CLIENT_SDK_SAMPLEAPP_INCLUDE_DIRECTIVE_HANDLER_H_
