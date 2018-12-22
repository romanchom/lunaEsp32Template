#pragma once

#include <cstdint>

extern uint8_t ca_cert[] asm("_binary_ca_cert_pem_start");
extern uint8_t ca_cert_end[] asm("_binary_ca_cert_pem_end");

extern uint8_t my_cert[] asm("_binary_my_cert_pem_start");
extern uint8_t my_cert_end[] asm("_binary_my_cert_pem_end");

extern uint8_t my_key[] asm("_binary_my_key_pem_start");
extern uint8_t my_key_end[] asm("_binary_my_key_pem_end");
