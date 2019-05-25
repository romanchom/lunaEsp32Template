#!/bin/bash

openssl ecparam \
    -genkey \
    -name prime256v1 \
    -outform DER \
    -out ca_key.der

openssl req \
    -x509 \
    -new \
    -nodes \
    -keyform DER \
    -key ca_key.der \
    -sha256 \
    -days 3650 \
    -subj "/C=PL/ST=./O=Luna/CN=luna" \
    -outform DER \
    -out ca_cert.der
