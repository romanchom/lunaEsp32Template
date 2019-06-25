#!/bin/bash

openssl ecparam \
    -genkey \
    -name prime256v1 \
    -outform PEM \
    -out ca_key.pem

openssl req \
    -x509 \
    -new \
    -nodes \
    -keyform PEM \
    -key ca_key.pem \
    -sha256 \
    -days 3650 \
    -subj "/C=PL/ST=./O=Luna/CN=luna" \
    -outform PEM \
    -out ca_cert.pem
