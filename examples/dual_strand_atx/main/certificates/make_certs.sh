#!/bin/bash

openssl ecparam \
    -genkey \
    -name prime256v1 \
    -outform PEM \
    -out $1_key.pem

openssl req \
    -new \
    -keyform PEM \
    -key $1_key.pem \
    -out csr

openssl x509 \
    -req \
    -inform PEM \
    -in csr \
    -CAform PEM \
    -CA ca_cert.pem \
    -CAkeyform PEM \
    -CAkey ca_key.pem \
    -CAcreateserial \
    -days 3650 \
    -sha256 \
    -outform PEM \
    -out $1_cert.pem
