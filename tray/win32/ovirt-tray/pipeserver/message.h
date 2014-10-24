#pragma once

struct MessageEntry {
    UINT32 Length;
    char const * Value;
};

struct MessageField {
    MessageEntry Key;
    MessageEntry Value;
};

struct Message {
    UINT32 BodySize;
    UINT32 FieldCount;
    MessageField * Fields;
};

