# 
# Copyright 2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
# 
# Licensed under the Apache License, Version 2.0 (the "License").
# You may not use this file except in compliance with the License.
# A copy of the License is located at
# 
#  http://aws.amazon.com/apache2.0
# 
# or in the "license" file accompanying this file. This file is distributed
# on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
# express or implied. See the License for the specific language governing
# permissions and limitations under the License.
# 

from flask import Flask, redirect, request
import requests
import json
import commentjson
import re

from os.path import abspath, isfile, dirname
import sys
from urllib import urlencode

# Shuts down the web-server.
def shutdown(): 
    func = request.environ.get('werkzeug.server.shutdown')
    if func is None:
        return 'You can close this window and terminate the script.'
    else:
        func()
        return 'Server is shutting down, so you can close this window.'

app = Flask(__name__)

# Static redirect_uri.
redirectUri = 'http://localhost:3000/authresponse'

# Static JSON key for the config file.
authDelegateKey = 'authDelegate'

# Amazon LWA API URL.
amazonLwaApiUrl = 'https://api.amazon.com/auth/o2/token'

# Amazon LWA API Request header.
amazonLwaApiHeaders = {'Content-Type': 'application/x-www-form-urlencoded'}

# Default configuration filename, to be filled by CMake
defaultConfigFilename = "${SDK_CONFIG_FILE_TARGET}"

# JSON keys for config file
CLIENT_ID = 'clientId'
CLIENT_SECRET = 'clientSecret'
PRODUCT_ID = 'productId'
DEVICE_SERIAL_NUMBER = 'deviceSerialNumber'
REFRESH_TOKEN = 'refreshToken'

# Read the configuration filename from the command line arguments -if it exists.
if 2 == len(sys.argv):
    configFilename = abspath(sys.argv[1])
else:
    # Assuming the script hasn't moved from the original location
    # use the default location of SDKConfig.json using relative paths
    configFilename = defaultConfigFilename

# Check if the configuration file exists.
if not isfile(configFilename):
    print 'The file "' + \
            configFilename + \
            '" does not exists. Please create this file and fill required data.'
    sys.exit(1)

try:
    configFile = open(configFilename,'r')

except IOError:
    print 'File "' + configFilename + '" not found!'
    sys.exit(1)
else:
    with configFile:
        configData = commentjson.load(configFile)
        if not configData.has_key(authDelegateKey):
            print 'The config file "' + \
                    configFilename + \
                    '" is missing the field "' + \
                    authDelegateKey + \
                    '".'
            sys.exit(1)
        else:
            authDelegateDict = configData[authDelegateKey]


# Check if all required keys are parsed.
requiredKeys = [CLIENT_ID, CLIENT_SECRET, PRODUCT_ID, DEVICE_SERIAL_NUMBER]
try:
    missingKey = requiredKeys[map(authDelegateDict.has_key,requiredKeys).index(False)];
    print 'Missing key: "' + missingKey + '". The list of required keys are:'
    print ' * ' + '\n * '.join(requiredKeys)
    print 'Exiting.'
    sys.exit(1)
except ValueError:
    pass

# Refresh the refresh token to check if it really is a refresh token.
if authDelegateDict.has_key(REFRESH_TOKEN):
    postData = {
            'grant_type': 'refresh_token',
            'refresh_token': authDelegateDict[REFRESH_TOKEN],
            'client_id': authDelegateDict[CLIENT_ID],
            'client_secret': authDelegateDict[CLIENT_SECRET]}
    tokenRefreshRequest = requests.post(
            amazonLwaApiUrl,
            data=urlencode(postData),
            headers=amazonLwaApiHeaders)
    defaultRefreshTokenString = authDelegateDict[REFRESH_TOKEN];
    if 200 == tokenRefreshRequest.status_code:
        print 'You have a valid refresh token already in the file.' 
        sys.exit(0)
    else:
        print 'The refresh request failed with the response code ' + \
                str(tokenRefreshRequest.status_code) + \
                ('. This might be due to a bad refresh token or bad client data. '
                'We will continue with getting a refresh token, discarding the one in the file.\n')
else:
    print 'Missing key: "' + REFRESH_TOKEN
    sys.exit(0)
# The top page redirects to LWA page.
@app.route('/')
def index():
    scopeData = ('{{"alexa:all":'
            '{{"productID":"{productId}",'
            '"productInstanceAttributes":'
            '{{"deviceSerialNumber":"{deviceSerialNumber}"}}}}}}').format(
                    productId=authDelegateDict[PRODUCT_ID],
                    deviceSerialNumber=authDelegateDict[DEVICE_SERIAL_NUMBER])
    lwaUrl = 'https://www.amazon.com/ap/oa/?' + urlencode({
        'scope': 'alexa:all',
        'scope_data': scopeData,
        'client_id': authDelegateDict[CLIENT_ID],
        'response_type': 'code',
        'redirect_uri': redirectUri,
        })
    return redirect(lwaUrl)

# The `authresponse` received from the redirect through LWA makes a POST to LWA API to get the token.
@app.route('/authresponse')
def get_refresh_token():
    postData = {
            'grant_type': 'authorization_code',
            'code': request.args.get('code',''),
            'client_id': authDelegateDict[CLIENT_ID],
            'client_secret': authDelegateDict[CLIENT_SECRET],
            'redirect_uri': redirectUri,
            }
    tokenRequest = requests.post(
            amazonLwaApiUrl,
            data=urlencode(postData),
            headers=amazonLwaApiHeaders)
    if not tokenRequest.json().has_key('refresh_token'):
        return 'LWA did not return a refresh token, with the response code ' + \
                str(tokenRequest.status_code) + \
                '.  This is most likely due to an error. Check the following JSON response:<br/>' + \
                tokenRequest.text + \
                '<br/>' + shutdown()
    authDelegateDict['refreshToken'] = tokenRequest.json()['refresh_token']
    try:
        configFile = open(configFilename,'r')
    except IOError:
        print 'File "' + configFilename + '" cannot be opened!'
        return '<h1>The file "' + \
                configFilename + \
                '" cannot be opened, please check if the file is open elsewhere.<br/>' + \
                shutdown() + \
                '</h1>'
    else:
        fileContent = configFile.read()
        try:
            configFile = open(configFilename,'w')
        except IOError:
            print 'File "' + configFilename + '" cannot be opened!'
            return '<h1>The file "' + \
                    configFilename + \
                    '" cannot be opened, please check if the file is open elsewhere.<br/>' + \
                    shutdown() + \
                    '</h1>'
        replaceWith = '\n\t\t\"refreshToken\":' + '\"' + tokenRequest.json()['refresh_token'] + '\",'
        stringToFind = r"\s*\"\s*{}\s*\"\s*:\s*\"\s*".format(REFRESH_TOKEN) + \
                        re.escape(defaultRefreshTokenString) + r"\s*\"\s*,"
        fileContent = re.sub(stringToFind, replaceWith, fileContent)
        configFile.write(fileContent)
        return '<h1>The file is written successfully.<br/>' + shutdown() + '</h1>'
    
app.run(host='127.0.0.1',port=3000)
