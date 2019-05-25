#!/bin/bash

openssl ecparam \
    -genkey \
    -name prime256v1 \
    -outform DER \
    -out $1_key.der

openssl req \
    -new \
    -keyform DER \
    -key $1_key.der \
    -out csr

openssl x509 \
    -req \
    -inform PEM \
    -in csr \
    -CAform DER \
    -CA ca_cert.der \
    -CAkeyform DER \
    -CAkey ca_key.der \
    -CAcreateserial \
    -days 3650 \
    -sha256 \
    -outform DER \
    -out $1_cert.der
