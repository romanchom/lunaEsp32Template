#pragma once

extern char const caCert[] asm("_binary_ca_cert_pem_start");
extern char const caCertEnd[] asm("_binary_ca_cert_pem_end");

extern char const myCert[] asm("_binary_my_cert_pem_start");
extern char const myCertEnd[] asm("_binary_my_cert_pem_end");

extern char const myKey[] asm("_binary_my_key_pem_start");
extern char const myKeyEnd[] asm("_binary_my_key_pem_end");
