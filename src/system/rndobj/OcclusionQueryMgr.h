#pragma once

class RndOcclusionQueryMgr {
public:
    RndOcclusionQueryMgr();
    virtual ~RndOcclusionQueryMgr() {}
    virtual void OnCreateQuery(unsigned int) = 0;
    virtual void OnBeginQuery(unsigned int) = 0;
    virtual void OnEndQuery(unsigned int) = 0;
    virtual bool OnGetQueryResult(unsigned int, unsigned int &) = 0;
    virtual bool OnIsValidQuery(unsigned int) const = 0;
    virtual void OnReleaseQuery(unsigned int, unsigned int &) = 0;
    virtual void OnBeginFrame() = 0;
    virtual void OnEndFrame() = 0;

    void ReleaseQuery(unsigned int);
};
