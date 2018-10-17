#!/bin/bash
openssl ecparam -genkey -name prime256v1 -out ca_key.pem
openssl ecparam -genkey -name prime256v1 -out my_key.pem
openssl ecparam -genkey -name prime256v1 -out cl_key.pem

openssl req -new -sha256 -key ca_key.pem -out csr
openssl req -x509 -sha256 -days 3650 -key ca_key.pem -in csr -out ca_cert.pem
rm csr

openssl req -new -sha256 -key my_key.pem -out csr
openssl req -x509 -sha256 -days 3650 -key ca_key.pem -in csr -out my_cert.pem
rm csr

openssl req -new -sha256 -key cl_key.pem -out csr
openssl req -x509 -sha256 -days 3650 -key ca_key.pem -in csr -out cl_cert.pem
rm csr

