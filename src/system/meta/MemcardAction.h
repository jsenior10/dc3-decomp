#pragma once

class Profile;

class MemcardAction {
public:
    MemcardAction(Profile *);
    virtual ~MemcardAction() {}

private:
    int unk4;
    int unk8;
    int unkc;
    Profile *unk10;
};
