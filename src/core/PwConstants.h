/*
  KeePass Password Safe - Qt Port
  Field type constants for KDB file format
  Copyright (C) 2003-2025 Dominik Reichl <dominik.reichl@t-online.de>
  Qt Port Copyright (C) 2025
*/

#ifndef PW_CONSTANTS_H
#define PW_CONSTANTS_H

// Group field types
#define GRP_FIELD_EXT_DATA     0x0000
#define GRP_FIELD_ID           0x0001
#define GRP_FIELD_NAME         0x0002
#define GRP_FIELD_CREATION     0x0003
#define GRP_FIELD_LASTMOD      0x0004
#define GRP_FIELD_LASTACCESS   0x0005
#define GRP_FIELD_EXPIRE       0x0006
#define GRP_FIELD_IMAGEID      0x0007
#define GRP_FIELD_LEVEL        0x0008
#define GRP_FIELD_FLAGS        0x0009
#define GRP_FIELD_END          0xFFFF

// Entry field types
#define ENT_FIELD_EXT_DATA     0x0000
#define ENT_FIELD_UUID         0x0001
#define ENT_FIELD_GROUPID      0x0002
#define ENT_FIELD_IMAGEID      0x0003
#define ENT_FIELD_TITLE        0x0004
#define ENT_FIELD_URL          0x0005
#define ENT_FIELD_USERNAME     0x0006
#define ENT_FIELD_PASSWORD     0x0007
#define ENT_FIELD_ADDITIONAL   0x0008
#define ENT_FIELD_CREATION     0x0009
#define ENT_FIELD_LASTMOD      0x000A
#define ENT_FIELD_LASTACCESS   0x000B
#define ENT_FIELD_EXPIRE       0x000C
#define ENT_FIELD_BINARYDESC   0x000D
#define ENT_FIELD_BINARYDATA   0x000E
#define ENT_FIELD_END          0xFFFF

// Flag definitions
#define PWM_FLAG_SHA2      1
#define PWM_FLAG_RIJNDAEL  2
#define PWM_FLAG_ARCFOUR   4
#define PWM_FLAG_TWOFISH   8

#endif // PW_CONSTANTS_H
