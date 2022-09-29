#!/bin/bash

# Copyright Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

# Small and simple script to create Certificate Authority (root CA), create client and server certificate requests, sign them and transform into OS consumable form.
# Please ensure  to change certificate subjects and ca_settings.conf/cert_settings.conf as per your policy. Existing values is for reference only.

openssl req -x509 -config ./ca_settings.cnf -new -newkey rsa:2048 -nodes -out ca.cert -keyout ca.key -days 7
openssl req -new -newkey rsa:2048 -nodes -out client.csr -keyout client.key -subj "/CN=MMSDK_Client_Cert/C=US/ST=WA/L=Seattle/O=Amazon"
openssl req -new -newkey rsa:2048 -nodes -out server.csr -keyout server.key -subj "/CN=localhost/C=US/ST=WA/L=Seattle/O=Amazon"
openssl x509 -req -in client.csr -CA ca.cert -CAkey ca.key -CAcreateserial -out client.cert -extfile ./cert_settings.cnf -extensions v3_req -sha256 -days 7
openssl x509 -req -in server.csr -CA ca.cert -CAkey ca.key -CAcreateserial -out server.cert -extfile ./cert_settings.cnf -extensions v3_req -sha256 -days 7
cat server.cert ca.cert > server.chain
cat client.cert ca.cert > client.chain
openssl pkcs12 -export -clcerts -inkey client.key -in client.chain -out client.p12 -name "client_cert_key"