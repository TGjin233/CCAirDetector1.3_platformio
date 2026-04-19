#ifndef _JWTUTIL_H
#define _JWTUTIL_H

#include <Arduino.h>

String generateJWT(char *PrivateKey, char *PublicKey, String KeyID, String ProjectID,unsigned long timestamp);

#endif