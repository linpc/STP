#include "PackedRow.h"
namespace MINISAT
{

std::ostream& operator << (std::ostream& os, const PackedRow& m)
{
    for(uint i = 0; i < m.size*64; i++) {
        os << m[i];
    }
    os << " -- xor: " << m.get_xor_clause_inverted();
    return os;
}

bool PackedRow::operator ==(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (std::equal(b.mp-1, b.mp+size, mp-1));
}

bool PackedRow::operator !=(const PackedRow& b) const
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    return (std::equal(b.mp-1, b.mp+size, mp-1));
}

bool PackedRow::popcnt_is_one() const
{
    char popcount = 0;
    for (uint i = 0; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        for (uint i2 = 0; i2 < 64; i2++) {
            popcount += tmp & 1;
            if (popcount > 1) return false;
            tmp >>= 1;
        }
    }
    return popcount;
}

bool PackedRow::popcnt_is_one(uint from) const
{
    from++;
    for (uint i = from/64; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint i2;
        if (i == from/64) {
            i2 = from%64;
            tmp >>= i2;
        } else
            i2 = 0;
        for (; i2 < 64; i2++) {
            if (tmp & 1) return false;
            tmp >>= 1;
        }
    }
    return true;
}

uint PackedRow::popcnt() const
{
    uint popcnt = 0;
    for (uint i = 0; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        for (uint i2 = 0; i2 < 64; i2++) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

uint PackedRow::popcnt(const uint from) const
{
    uint popcnt = 0;
    for (uint i = from/64; i < size; i++) if (mp[i]) {
        uint64_t tmp = mp[i];
        uint i2;
        if (i == from/64) {
            i2 = from%64;
            tmp >>= i2;
        } else
            i2 = 0;
        for (; i2 < 64; i2++) {
            popcnt += (tmp & 1);
            tmp >>= 1;
        }
    }
    return popcnt;
}

PackedRow& PackedRow::operator=(const PackedRow& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(size == b.size);
    #endif
    
    memcpy(mp-1, b.mp-1, size+1);
    return *this;
}

PackedRow& PackedRow::operator^=(const PackedRow& b)
{
    #ifdef DEBUG_ROW
    assert(size > 0);
    assert(b.size > 0);
    assert(b.size == size);
    #endif
    
    for (uint i = 0; i < size; i++) {
        mp[i] ^= b.mp[i];
    }
    xor_clause_inverted ^= !b.xor_clause_inverted;
    return *this;
}

void PackedRow::fill(Lit* ps, const vec<lbool>& assigns, const vector<Var>& col_to_var_original) const
{
    bool final = xor_clause_inverted;
    
    Lit* ps_first = ps;
    uint col = 0;
    bool wasundef = false;
    for (uint i = 0; i < size; i++) for (uint i2 = 0; i2 < 64; i2++) {
        if ((mp[i] >> i2) &1) {
            const uint& var = col_to_var_original[col];
            assert(var != UINT_MAX);
            
            const lbool val = assigns[var];
            const bool val_bool = val.getBool();
            *ps = Lit(var, val_bool);
            final ^= val_bool;
            if (val.isUndef()) {
                assert(!wasundef);
                Lit tmp(*ps_first);
                *ps_first = *ps;
                *ps = tmp;
                wasundef = true;
            }
            ps++;
        }
        col++;
    }
    if (wasundef) {
        *ps_first ^= final;
        //assert(ps != ps_first+1);
    } else
        assert(!final);
}
};
