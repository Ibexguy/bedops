/*
 Author : Shane Neph & Scott Kuehn
 Date   : Fri Aug 24 18:12:08 PDT 2007
*/
//
//    BEDOPS
//    Copyright (C) 2011-2016 Shane Neph, Scott Kuehn and Alex Reynolds
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//

#ifndef BED_DATA_COMPARE_H
#define BED_DATA_COMPARE_H

#include <cstring>
#include <functional>
#include <limits>
#include <type_traits>

namespace Bed {

  // Expect predicate function objects to be defined here

  template <typename BedType1, typename BedType2 = BedType1>
  struct GenomicCompare
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    inline bool operator()(BedType1 const* ptr1, BedType2 const* ptr2) const {
      static int v = 0;
      if ( (v = std::strcmp(ptr1->chrom(), ptr2->chrom())) != 0 )
        return v < 0;
      if ( ptr1->start() != ptr2->start() )
        return ptr1->start() < ptr2->start();
      return ptr1->end() < ptr2->end();
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct GenomicAddressCompare
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* ptr1, BedType2 const* ptr2) const {
      static int v = 0;
      if ( (v = std::strcmp(ptr1->chrom(), ptr2->chrom())) != 0 )
        return v < 0;
      if ( ptr1->start() != ptr2->start() )
        return ptr1->start() < ptr2->start();
      else if ( ptr1->end() != ptr2->end() )
        return ptr1->end() < ptr2->end();
      return ptr1 < ptr2;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct InvertGenomicAddressCompare
      : private GenomicAddressCompare<BedType1, BedType2> {
    typedef GenomicAddressCompare<BedType1, BedType2> Base;
    bool operator()(BedType1 const* ptr1, BedType2 const* ptr2) const {
      return Base::operator()(ptr2, ptr1);
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct CoordCompare // ignoring chrom here
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      return one->end() < two->end();
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct CoordAddressCompare // not caring about chrom here
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      return one < two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct CoordRestCompare // not caring about chrom here
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<T::UseRest && U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      return std::strcmp(one->rest(), two->rest()) < 0;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<T::UseRest && !U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      return std::strlen(one->rest()) == 0;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<!T::UseRest && U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      std::strlen(two->rest()) != 0;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<!T::UseRest && !U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      return one->end() < two->end();
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct CoordRestAddressCompare // not caring about chrom here
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<T::UseRest && U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      int val = std::strcmp(one->rest(), two->rest());
      if ( val != 0 )
        return val < 0;
      return one < two;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<T::UseRest && !U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      auto n = std::strlen(one->rest());
      if ( n != 0 )
        return false;
      return one < two;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<!T::UseRest && U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      auto n = std::strlen(two->rest());
      if ( n != 0 )
        return true;
      return one < two;
    }

    template <typename T=BedType1, typename U=BedType2>
    typename std::enable_if<!T::UseRest && !U::UseRest, bool>::type
    operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      if ( one->end() != two->end() )
        return one->end() < two->end();
      return one < two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct RevCoordAddressCompare // not caring about chrom here
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->end() != two->end() )
        return one->end() < two->end();
      if ( one->start() != two->start() )
        return one->start() < two->start();
      return one < two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct EndCoordAddressCompareLesser
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->end() != two->end() )
        return one->end() < two->end();
      return one < two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct EndCoordAddressCompareGreater
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->end() != two->end() )
        return one->end() > two->end();
      return one > two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct StartCoordAddressCompareLesser
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() < two->start();
      return one < two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct StartCoordAddressCompareGreater
    : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->start() != two->start() )
        return one->start() > two->start();
      return one > two;
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct ScoreCompare
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {
    
    bool operator()(BedType1 const* one, BedType2 const* two) const {
      return one->measurement() < two->measurement();
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct ScoreThenGenomicCompareLesser
     : public std::binary_function<BedType1 const*, BedType2 const*, bool> {

    inline
    bool operator()(BedType1 const* one, BedType2 const* two) const {
      if ( one->measurement() != two->measurement() )
        return one->measurement() < two->measurement();

      static int v = 0;
      if ( (v = std::strcmp(one->chrom(), two->chrom())) != 0 )
        return v < 0;
      if ( one->start() != two->start() )
        return one->start() < two->start();
      return one->end() < two->end();
    }
  };

  template <typename BedType1, typename BedType2 = BedType1>
  struct ScoreThenGenomicCompareGreater
      : private ScoreThenGenomicCompareLesser<BedType1, BedType2> {
    typedef ScoreThenGenomicCompareLesser<BedType1, BedType2> Base;
    inline
    bool operator()(BedType1 const* ptr1, BedType2 const* ptr2) const {
      return Base::operator()(ptr2, ptr1);
    }
  };

} // namespace Bed



#endif // BED_DATA_COMPARE_H
