// -*- mode:C++; tab-width:8; c-basic-offset:2; indent-tabs-mode:t -*- 
// vim: ts=8 sw=2 smarttab
/*
 * Ceph - scalable distributed file system
 *
 * Copyright (C) 2004-2006 Sage Weil <sage@newdream.net>
 *
 * This is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License version 2.1, as published by the Free Software 
 * Foundation.  See file COPYING.
 * 
 */

#ifndef __MDS_TYPES_H
#define __MDS_TYPES_H

#include "ceph_fs.h"

extern "C" {
#include <stdint.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include "statlite.h"
}

#include <string>
#include <list>
#include <set>
#include <map>
#include <vector>
#include <iostream>
#include <iomanip>

using namespace std;

#include <ext/hash_map>
using namespace __gnu_cxx;



#include "object.h"
#include "utime.h"
#include "intarith.h"

#include "../acconfig.h"

// DARWIN compatibility
#ifdef DARWIN
typedef long long loff_t;
typedef long long off64_t;
#define O_DIRECT 00040000
#endif

// -- stl crap --

namespace __gnu_cxx {
  template<> struct hash< std::string >
  {
    size_t operator()( const std::string& x ) const
    {
      static hash<const char*> H;
      return H(x.c_str());
    }
  };

#ifndef __LP64__
  template<> struct hash<int64_t> {
    size_t operator()(int64_t __x) const { 
      static hash<int32_t> H;
      return H((__x >> 32) ^ (__x & 0xffffffff)); 
    }
  };
  template<> struct hash<uint64_t> {
    size_t operator()(uint64_t __x) const { 
      static hash<uint32_t> H;
      return H((__x >> 32) ^ (__x & 0xffffffff)); 
    }
  };
#endif

}


/*
 * comparators for stl containers
 */
// for hash_map:
//   hash_map<const char*, long, hash<const char*>, eqstr> vals;
struct eqstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) == 0;
  }
};

// for set, map
struct ltstr
{
  bool operator()(const char* s1, const char* s2) const
  {
    return strcmp(s1, s2) < 0;
  }
};



#include "encoding.h"

WRITE_RAW_ENCODER(ceph_fsid)
WRITE_RAW_ENCODER(ceph_file_layout)
WRITE_RAW_ENCODER(ceph_mds_request_head)
WRITE_RAW_ENCODER(ceph_mds_file_caps)
WRITE_RAW_ENCODER(ceph_mds_lease)
WRITE_RAW_ENCODER(ceph_mds_reply_head)
WRITE_RAW_ENCODER(ceph_mds_reply_inode)
WRITE_RAW_ENCODER(ceph_frag_tree_split)
WRITE_RAW_ENCODER(ceph_inopath_item)

WRITE_RAW_ENCODER(ceph_osd_request_head)
WRITE_RAW_ENCODER(ceph_osd_reply_head)

WRITE_RAW_ENCODER(ceph_statfs)

// ----------------------
// some basic types

// NOTE: these must match ceph_fs.h typedefs
typedef __u64 tid_t;         // transaction id
typedef __u64 version_t;
typedef __u32 epoch_t;       // map epoch  (32bits -> 13 epochs/second for 10 years)


#define O_LAZY 01000000

typedef __u64 coll_t;


// --------------------------------------
// ino

typedef __u64 _inodeno_t;

struct inodeno_t {
  _inodeno_t val;
  inodeno_t() : val(0) {}
  inodeno_t(_inodeno_t v) : val(v) {}
  inodeno_t operator+=(inodeno_t o) { val += o.val; return *this; }
  operator _inodeno_t() const { return val; }
};

inline void encode(inodeno_t i, bufferlist &bl) { encode(i.val, bl); }
inline void decode(inodeno_t &i, bufferlist::iterator &p) { decode(i.val, p); }

inline ostream& operator<<(ostream& out, inodeno_t ino) {
  return out << hex << ino.val << dec;
}

namespace __gnu_cxx {
  template<> struct hash< inodeno_t >
  {
    size_t operator()( const inodeno_t& x ) const
    {
      static rjhash<uint64_t> H;
      return H(x.val);
    }
  };
}


// file modes

static inline bool file_mode_is_readonly(int mode) {
  return (mode & CEPH_FILE_MODE_WR) == 0;
}

inline int DT_TO_MODE(int dt) {
  return dt << 12;
}

inline unsigned char MODE_TO_DT(int mode) {
  return mode >> 12;
}



// dentries
#define MAX_DENTRY_LEN 255


// --

inline ostream& operator<<(ostream& out, ceph_fsid& f) {
  return out << hex << f.major << '.' << f.minor << dec;
}



// -- io helpers --

template<class A, class B>
inline ostream& operator<<(ostream& out, pair<A,B> v) {
  return out << v.first << "," << v.second;
}

template<class A>
inline ostream& operator<<(ostream& out, vector<A>& v) {
  out << "[";
  for (unsigned i=0; i<v.size(); i++) {
    if (i) out << ",";
    out << v[i];
  }
  out << "]";
  return out;
}

template<class A>
inline ostream& operator<<(ostream& out, const list<A>& ilist) {
  for (typename list<A>::const_iterator it = ilist.begin();
       it != ilist.end();
       it++) {
    if (it != ilist.begin()) out << ",";
    out << *it;
  }
  return out;
}

template<class A>
inline ostream& operator<<(ostream& out, const set<A>& iset) {
  for (typename set<A>::const_iterator it = iset.begin();
       it != iset.end();
       it++) {
    if (it != iset.begin()) out << ",";
    out << *it;
  }
  return out;
}

template<class A>
inline ostream& operator<<(ostream& out, const multiset<A>& iset) {
  for (typename multiset<A>::const_iterator it = iset.begin();
       it != iset.end();
       it++) {
    if (it != iset.begin()) out << ",";
    out << *it;
  }
  return out;
}

template<class A,class B>
inline ostream& operator<<(ostream& out, const map<A,B>& m) 
{
  out << "{";
  for (typename map<A,B>::const_iterator it = m.begin();
       it != m.end();
       it++) {
    if (it != m.begin()) out << ",";
    out << it->first << "=" << it->second;
  }
  out << "}";
  return out;
}



#endif
