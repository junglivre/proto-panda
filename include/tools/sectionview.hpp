#pragma once
#include <cstdint>
#include <cstring>
#include <Arduino.h>

#define MAX_VIEWS 32

template<int V> class SectionMap {
public:

    struct View {
        int dispX, dispY;
        int srcX,  srcY;
        int w,     h;
        bool flip_horizontal, flip_vertical;
    };

    SectionMap() : m_count(0) {}

    void addView(int dispX, int dispY, int srcX,  int srcY, int w, int h, bool fliph, bool flipv) {
        if (m_count < V) {
            View& v = m_views[m_count++];
            v.dispX = dispX;   
            v.dispY = dispY;
            v.srcX  = srcX;  
            v.srcY  = srcY; 
            v.w     = w;      
            v.h     = h;
            v.flip_horizontal = fliph;
            v.flip_vertical = flipv;
        }
    }
    
    void setView(int id, int dispX, int dispY, int srcX,  int srcY, int w, int h, bool fliph, bool flipv) {
        if (m_count > id) {
            View& v = m_views[id];
            v.dispX = dispX;   
            v.dispY = dispY;
            v.srcX  = srcX;  
            v.srcY  = srcY; 
            v.w     = w;      
            v.h     = h;
            v.flip_horizontal = fliph;
            v.flip_vertical = flipv;
        }
    }

    bool getSize(int id, int& ww, int& hh){
        if (id >= m_count) {
            return false;
        }
        const View& v = m_views[id];
        hh = v.h;
        ww = v.w;
        return true;
    }

    inline bool getPosition(int x, int y, int& tx, int& ty) const {
        if (m_count == 0) {
            tx = x; ty = y;
            return true;
        }
        const View* v   = m_views;
        const View* end = v + m_count;
        for (; v != end; ++v) {
            int dx = x - v->srcX;
            int dy = y - v->srcY;
            if (dx >= 0 && dy >= 0 && dx < v->w && dy < v->h) {
                if (v->flip_horizontal) dx = (v->w - 1) - dx;
                if (v->flip_vertical)   dy = (v->h - 1) - dy;
                tx = v->dispX + dx;
                ty = v->dispY + dy;
                return true;
            }
        }
        return false;
    }

    void clear()       { m_count = 0; }
    int  count() const { return m_count; }

private:
    View m_views[V];
    int  m_count;
};