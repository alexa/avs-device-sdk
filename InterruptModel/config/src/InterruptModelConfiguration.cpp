/*
 * Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * SPDX-License-Identifier: LicenseRef-.amazon.com.-AmznSL-1.0
 * Licensed under the Amazon Software License (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *     http://aws.amazon.com/asl/
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#include "InterruptModel/config/InterruptModelConfiguration.h"

namespace alexaClientSDK {
namespace afml {
namespace interruptModel {

std::string InterruptModelConfiguration::configurationJsonDuckingNotSupported = R"( {
        "virtualChannels":{
            "audioChannels" : [
                {
                    "name" : "Earcon",
                    "priority" : 250
                },
                {
                    "name" : "HighPriorityDucking",
                    "priority" : 175
                }
            ],
            "visualChannels" : [

            ]
        },
        "interruptModel" : {
            "Dialog" : {
            },
            "Communications" : {
                "contentType":
                {
                    "MIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    },
                    "NONMIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    }
                }
            },
            "HighPriorityDucking" : {
                "contentType":
                {
                    "MIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Communications" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    }
                }
            },
            "Alert" : {
                "contentType" :
                {
                    "MIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                  "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Communications" : {
                                  "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "HighPriorityDucking" : {
                                  "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    }
                }
            },
            "Content" : {
                "contentType" :
                {
                    "MIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Communications" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Alert" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Earcon" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "HighPriorityDucking" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    },
                    "NONMIXABLE" : {
                        "incomingChannel" : {
                            "Dialog" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Communications" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Alert" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "Earcon" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE",
                                    "NONMIXABLE" : "MUST_PAUSE"
                                }
                            },
                            "HighPriorityDucking" : {
                                "incomingContentType" : {
                                    "MIXABLE" : "MUST_PAUSE"
                                }
                            }
                        }
                    }
                }
            }
        }
    })";

std::string InterruptModelConfiguration::configurationJsonSupportsDucking = R"( {
            "virtualChannels":{
                "audioChannels" : [
                    {
                        "name" : "Earcon",
                        "priority" : 250
                    },
                    {
                       "name" : "HighPriorityDucking",
                        "priority" : 175
                    }
                ],
                "visualChannels" : [

                ]
            },
            "interruptModel" : {
                "Dialog" : {
                },
                "Communications" : {
                    "contentType":
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                },
                "HighPriorityDucking" : {
                    "contentType":
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                },
                "Alert" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "HighPriorityDucking" : {
                                      "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        }
                    }
                },
                "Content" : {
                    "contentType" :
                    {
                        "MIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                },
                                "Earcon" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "HighPriorityDucking" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MAY_DUCK"
                                    }
                                }
                            }
                        },
                        "NONMIXABLE" : {
                            "incomingChannel" : {
                                "Dialog" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Communications" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Alert" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "Earcon" : {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE",
                                        "NONMIXABLE" : "MUST_PAUSE"
                                    }
                                },
                                "HighPriorityDucking": {
                                    "incomingContentType" : {
                                        "MIXABLE" : "MUST_PAUSE"
                                    }
                                }
                            }
                        }
                    }
                }
            }
        })";

}  // namespace interruptModel
}  // namespace afml
}  // namespace alexaClientSDK
