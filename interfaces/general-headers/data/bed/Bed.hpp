/*
  Author: Scott Kuehn, Shane Neph
  Date:   Fri Jul 27 11:49:03 PDT 2007
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

#ifndef BED_NONEW_HPP
#define BED_NONEW_HPP

#include <cstddef>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <limits>
#include <string>
#include <type_traits>

#include "suite/BEDOPS.Constants.hpp"
#include "utility/Formats.hpp"

/*
  sjn
  This set of classes is designed to be flexible and efficient.  I use non-virtual public inheritence, which
    most everyone will tell you is a horrible thing to do.  I disagree.  Power over safety is the bread and
    butter of the class of C languages.  If you try using derived classes through pointer to base class, then
    you are using these classes incorrectly.  You can allocate dynamically, of course, but the pointer type
    must be to the class of interest.  Not every problem should be solved through virtual tools, which carry
    some real overhead.  Why throw out power offered by the language by sticking to simple rules of thumb?
    Instead, use all tools wisely.
  There is no automated way (that I know of) to prevent improper base pointer to derived class.  Using these
    in that way leads to memory leaks (and is the reason for that rule of thumb).
*/

namespace Bed {

  const CoordType MAXCHROMSIZE = TOKEN_CHR_MAX_LENGTH;
  const CoordType MAXIDSIZE = TOKEN_ID_MAX_LENGTH;
  const CoordType MAXRESTSIZE = TOKEN_REST_MAX_LENGTH;


  /*****************************************/
  /* ChromInfo Classes                     */
  /*****************************************/

  // ChromInfo non-specialized declaration
  template <bool isNonStatic>
  struct ChromInfo {
    static constexpr bool IsNonStatic = isNonStatic;

    ChromInfo() { chrom_[0] = '\0'; }
    ChromInfo(const ChromInfo& c) { std::strcpy(chrom_, c.chrom_); }
    explicit ChromInfo(char const* c)  { std::strcpy(chrom_, c); }

    // Properties
    inline char const* chrom() const { return chrom_; }
    inline void chrom(char const* chrom) { std::strcpy(chrom_, chrom); }

    // Operators
    ChromInfo& operator=(const ChromInfo& c) {
      std::strcpy(chrom_, c.chrom_);
      return *this;
    }

  protected:
    char chrom_[MAXCHROMSIZE+1];
  };


  // ChromInfo specialization : uses static chromosome info
  template <>
  struct ChromInfo<false> {

    ChromInfo() { chrom_[0] = '\0'; }
    explicit ChromInfo(char const* chr) { std::strcpy(chrom_, chr); }
    /* Desire default-shallow copy-construction & copy-assignment */

    // Properties
    inline char const* chrom() const { return chrom_; }
    inline void chrom(char const* chrom) { std::strcpy(chrom_, chrom); }

  protected:
    static char chrom_[MAXCHROMSIZE+1];
  };

  char ChromInfo<false>::chrom_[MAXCHROMSIZE+1];


  /*****************************************/
  /* BasicCoords Classes                   */
  /*****************************************/

  // Forward declaration
  template <bool IsNonStaticChrom = true, bool HasRest = false>
  struct BasicCoords;


  // BasicCoords specialization 1: vanilla (default) information
  template <bool IsNonStaticChrom>
  struct BasicCoords<IsNonStaticChrom, false> : public ChromInfo<IsNonStaticChrom> {

    BasicCoords() : BaseClass(), start_(1), end_(0) { /* invalid coords by default */ }
    BasicCoords(char const* chrom, CoordType start, CoordType end)
      : BaseClass(chrom), start_(start), end_(end) {}
    BasicCoords(const BasicCoords& c)
      : BaseClass(c.chrom_), start_(c.start_), end_(c.end_) {}
    explicit BasicCoords(FILE* inF) : BaseClass()
      { this->readline(inF); }
    explicit BasicCoords(const std::string& inS) : BaseClass()
      { this->readline(inS); }

    // Properties
    inline CoordType length() const { return end_ - start_; }
    inline CoordType median() const { return start_ + ((end_ - start_) / 2); }
    inline CoordType distance(const BasicCoords& a) const {
      if ( 0 == std::strcmp(chrom_, a.chrom_) )
        return start_ - a.start_;
      return std::numeric_limits<CoordType>::max();
    }
    inline CoordType sepDistance(const BasicCoords& a) const {
      if( 0 == std::strcmp(chrom_, a.chrom_) )
        return end_ - a.start_;
      return std::numeric_limits<CoordType>::max();
    }
    inline void start(CoordType start) { start_ = start; }
    inline CoordType start() const { return start_; }
    inline void end(CoordType end) { end_ = end; }
    inline CoordType end() const { return end_; }

    // Comparison utilities
    inline CoordType overlap(const BasicCoords& a) const {
      if ( 0 != std::strcmp(chrom_, a.chrom_) )
        return 0;
      if ( start_ >= a.start_ ) {
        if ( a.end_ > start_ ) {
          if ( a.end_ > end_ )
            return end_ - start_;
          return a.end_ - start_;
        }
        else
          return 0;
      } else {
        if ( end_ > a.start_ ) {
          if ( a.end_ < end_ )
            return a.end_ - a.start_;
          return end_ - a.start_;
        }
        else
         return 0;
      }
    }
    BasicCoords& intersection(const BasicCoords& a) {
      if ( overlap(a) ) {
        start_ = std::max(start_, a.start_);
        end_ = std::min(end_, a.end_);
      }
      else
        start_ = end_ = 0;
      return *this;
    }
    BasicCoords& eunion(const BasicCoords& a) {
      if ( overlap(a) ) {
        start_ = std::min(start_, a.start_);
        end_ = std::max(end_, a.end_);
      }
      else
        start_ = end_ = 0;
      return *this;
    }
    static bool lengthCompare(const BasicCoords& a, const BasicCoords& b)
      { return a.length() < b.length(); }

    // Operators
    BasicCoords& operator=(const BasicCoords& c) {
      BaseClass::operator=(c);
      start_ = c.start_;
      end_ = c.end_;
      return *this;
    }

    // IO
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_);
    }
    inline void println() const {
      static const std::string heapFormat = (outFormatter() + "\n");
      static char const* format = heapFormat.c_str();
      std::printf(format, chrom_, start_, end_);
    }
    inline int readline(const std::string& inputLine) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      *chrom_ = '\0';
      int numScan = std::sscanf(inputLine.c_str(),
                                format, chrom_,
                                &start_, &end_);
      return numScan;
    }
    inline int readline(FILE* inputFile) {
      static std::string lclstatic = inFormatter();
      static char const* format = lclstatic.c_str();
      *chrom_ = '\0';
      int numScan = std::fscanf(inputFile,
                                format, chrom_,
                                &start_, &end_);
      std::fgetc(inputFile); // chomp newline
      return numScan;
    }

    static const int NumFields = 3;
    static const bool UseRest = false;

  protected:
    typedef ChromInfo<IsNonStaticChrom> BaseClass;
    using BaseClass::chrom_;
    CoordType start_;
    CoordType end_;

    static std::string outFormatter() {
      return(std::string("%s\t%" PRIu64 "\t%" PRIu64));
    }

    static std::string inFormatter() {
      return(std::string("%s\t%" SCNu64 "\t%" SCNu64 "%*[^\n]s\n"));
    }
  };


  // Non-Specialization: Extend partial specialization with "rest-size" information
  template <bool IsNonStaticChrom, bool HasRest>
  struct BasicCoords
    : public BasicCoords<IsNonStaticChrom, false> {

    BasicCoords() : BaseClass() { rest_[0] = '\0'; }
    BasicCoords(char const* chrom, CoordType start, CoordType end, char const* rest = NULL)
      : BaseClass(chrom, start, end) {
      rest_[0] = '\0';
      if ( rest && std::strcmp(rest, "") != 0 ) {
        if ( rest[0] != '\t' )
          rest_[0] = '\t';
        std::strcat(rest_, rest);
      }
    }
    BasicCoords(const BasicCoords& c)
      : BaseClass(c)
      { std::strcpy(rest_, c.rest_); }
    explicit BasicCoords(FILE* inF) : BaseClass()
      { this->readline(inF); }
    explicit BasicCoords(const std::string& inS) : BaseClass()
      { this->readline(inS); }

    // Properties
    inline char const* rest() const { return rest_; }

    // Operators
    BasicCoords& operator=(const BasicCoords& c) {
      BaseClass::operator=(c);
      std::strcpy(rest_, c.rest_);
      return *this;
    }

    // IO
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline void println() const {
      static const std::string heapFormat = (outFormatter() + "\n");
      static char const* format = heapFormat.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline int readline(const std::string& inputLine) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      chrom_[0]='\0';
      rest_[0]='\0'; // required
      int numScan = std::sscanf(inputLine.c_str(), 
                                format, chrom_, &start_,
                                &end_, rest_);
      return numScan;
    }
    inline int readline(FILE* inputFile) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      chrom_[0]='\0';
      rest_[0]='\0'; // required
      int numScan = std::fscanf(inputFile, format,
                                chrom_, &start_,
                                &end_, rest_);
      std::fgetc(inputFile); // chomp newline
      return numScan;
    }

    static const bool UseRest = true;

  protected:
    typedef BasicCoords<IsNonStaticChrom, false> BaseClass;
    using BaseClass::chrom_;
    using BaseClass::start_;
    using BaseClass::end_;
    char rest_[MAXRESTSIZE+1];

    static std::string outFormatter() {
      return(std::string("%s\t%" PRIu64 "\t%" PRIu64 "%s"));
    }

    static std::string inFormatter() {
      return(std::string("%s\t%" SCNu64 "\t%" SCNu64 "%[^\n]s\n"));
    }
  };
  

  /*****************************************/
  /* Bed4 Classes                         */
  /*****************************************/

  // Forward declaration, forcing specialization
  template <typename BedType, bool HasRest = false>
  struct Bed4;

  // Bed4 specialization 1: vanilla (default) information
  template <bool IsNonStaticChrom, bool B3HasRest>
  struct Bed4<BasicCoords<IsNonStaticChrom, B3HasRest>, false> 
    : public BasicCoords<IsNonStaticChrom, false> {

    Bed4() : BaseClass() { *id_ = '\0'; }
    Bed4(char const* chrom, CoordType start, CoordType end, char const* id)
      : BaseClass(chrom, start, end)
      { *id_ = '\0'; if ( id != NULL ) std::strcpy(id_, id); }
    Bed4(const Bed4& c)
      : BaseClass(c)
      { *id_ = '\0'; if ( c.id_ != NULL ) std::strcpy(id_, c.id_); }
    explicit Bed4(FILE* inF) : BaseClass()
      { this->readline(inF); }
    explicit Bed4(const std::string& inS) : BaseClass()
      { this->readline(inS); }

    // IO
    inline int readline(const std::string& inputLine) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      *chrom_ = '\0';
      *id_ = '\0';
      return std::sscanf(inputLine.c_str(), format,
                         chrom_, &start_, &end_, id_);
    }
    inline int readline(FILE* inputFile) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      *chrom_ = '\0';
      *id_ = '\0';
      int numScanned = std::fscanf(inputFile, format,
                                   chrom_, &start_, &end_, id_);
      std::fgetc(inputFile); // read and discard newline
      return numScanned;
    }
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_, id_);
    }
    inline void println() const {
      static const std::string heapFormat = (outFormatter() + "\n");
      static char const* format = heapFormat.c_str();
      printf(format, chrom_, start_, end_, id_);
    }

    // Properties
    inline void id(char const* id) { *id_ = '\0'; if ( id != NULL ) std::strcpy(id_, id); }
    inline char const* id() const { return id_; }

    // Operators
    Bed4& operator=(const Bed4& c) {
      BaseClass::operator=(c);
      *id_ = '\0'; if ( c.id_ != NULL ) std::strcpy(id_, c.id_);
      return *this;
    }

    static const int NumFields = 4;
    static const bool UseRest = false;

  protected:
    typedef BasicCoords<IsNonStaticChrom, false> BaseClass;
    using BaseClass::chrom_;
    using BaseClass::start_;
    using BaseClass::end_;

    char id_[MAXIDSIZE+1];

    static std::string outFormatter() {
      return(BaseClass::outFormatter() + "\t%s");
    }

    static std::string inFormatter() {
      return(outFormatter() + "%*[^\n]s\n");
    }
  };

  // Specialization 2: Extend specialization 1 with "rest-size" information
  template <bool IsNonStaticChrom, bool HasRest>
  struct Bed4<BasicCoords<IsNonStaticChrom, HasRest>, HasRest>
    : public BasicCoords<IsNonStaticChrom, HasRest> {

    Bed4() : BaseClass() { *rest_ = '\0'; }
    Bed4(const Bed4& c)
      : BaseClass(c)
       {
         *id_ = '\0';
         if ( c.id_ != NULL ) std::strcpy(id_, c.id_);
       }
    Bed4(char const* chrom, CoordType start, CoordType end, char const* id, char const* rest = NULL)
      : BaseClass(chrom, start, end, rest) {
      *id_ = '\0';
      if ( id && std::strcmp(id, "") != 0 )
        std::strcpy(id_, id);
    }
    explicit Bed4(FILE* inF) : BaseClass()
      { this->readline(inF); }
    explicit Bed4(const std::string& inS) : BaseClass()
      { this->readline(inS); }

    // Properties
    inline void id(char const* id) { *id_ = '\0'; if ( id != NULL ) std::strcpy(id_, id); }
    inline char const* id() const { return id_; }

    // IO
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline void println() const {
      static const std::string heapFormat = (outFormatter() + "\n");
      static char const* format = heapFormat.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline int readline(const std::string& inputLine) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      chrom_[0]='\0';
      id_[0]='\0';
      rest_[0]='\0';
      int numScanned = std::sscanf(inputLine.c_str(),
                                   format, chrom_,
                                   &start_, &end_, rest_);
      std::sscanf(rest_, "%s", id_);
      return numScanned;
    }
    inline int readline(FILE* inputFile) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      chrom_[0] = '\0';
      id_[0] = '\0';
      rest_[0] = '\0';
      int numScanned = std::fscanf(inputFile,
                                   format, chrom_,
                                   &start_, &end_,
                                   rest_);
      std::fgetc(inputFile); // Read and discard trailing newline
      std::sscanf(rest_, "%s%*s", id_);
      return numScanned;
    }

    // Operators
    Bed4& operator=(const Bed4& c) {
      BaseClass::operator=(c);
      *id_ = '\0'; if ( c.id_ != NULL ) std::strcpy(id_, c.id_);
      return *this;
    }

    static const int NumFields = 4;
    static const bool UseRest = true;

  protected:
    typedef BasicCoords<IsNonStaticChrom, HasRest> BaseClass;
    using BaseClass::chrom_;
    using BaseClass::start_;
    using BaseClass::end_;
    using BaseClass::rest_;

    char id_[MAXIDSIZE+1];

    static std::string outFormatter() {
      return(BaseClass::outFormatter());
    }

    static std::string inFormatter() {
      return(BaseClass::inFormatter());
    }
  };


  /*****************************************/
  /* Bed5 Classes                          */
  /*****************************************/

  // Forward declaration, forcing specialization
  template <typename Bed4Type, typename MeasureType, bool HasRest = false>
  struct Bed5;

  // Bed5 specialization 1: vanilla (default) information
  template <typename U, bool Bed4HasRest, typename MeasureType>
  struct Bed5<Bed4<U, Bed4HasRest>, MeasureType, false>
    : public Bed4<U, false> {

    Bed5() : BaseClass() {}
    Bed5(const Bed5& c) : BaseClass(c), measurement_(0) {}
    Bed5(char const* chrom, CoordType start, CoordType end, char const* id, MeasureType measurement)
        : BaseClass(chrom, start, end, id), measurement_(measurement) {}
    explicit Bed5(const std::string& inS) : BaseClass(), measurement_(0)
      { this->readline(inS); }
    explicit Bed5(FILE* inF) : BaseClass(), measurement_(0)
      { this->readline(inF); }

    // Parameters
    typedef MeasureType MeasurementType;
    static const int NumFields = 5;
    static const bool UseRest = false;

    // Properties
    inline MeasurementType measurement() const { return measurement_; }

    // IO
    inline int readline(const std::string& inputLine) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      *chrom_ = '\0';
      *id_ = '\0';
      return std::sscanf(inputLine.c_str(),
                         format, chrom_,
                         &start_, &end_, id_, 
                         &measurement_);
    }
    inline int readline(FILE* inputFile) {
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      *chrom_ = '\0';
      *id_ = '\0';
      int numScanned = std::fscanf(inputFile,
                                   format, chrom_,
                                   &start_, &end_, id_, measurement_);
      std::fgetc(inputFile); // Read and discard trailing newline
      return numScanned;
    }
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_, id_, measurement_);
    }
    inline void println() const {
      static const std::string heapFormat = outFormatter() + "\n";
      static char const* format = heapFormat.c_str();
      std::printf(format, chrom_, start_, end_, id_, measurement_);
    }

    // Operators
    Bed5& operator=(const Bed5& c) {
      BaseClass::operator=(c);
      this->measurement_ = c.measurement_;
      return *this;
    }
    inline operator MeasurementType() const { return measurement_; }

  protected:
    typedef Bed4<U, false> BaseClass;
    using BaseClass::chrom_;
    using BaseClass::start_;
    using BaseClass::end_;
    using BaseClass::id_;
    MeasureType measurement_;

    static std::string outFormatter() {
      typedef typename std::remove_cv<MeasureType>::type MType;
      return(BaseClass::outFormatter() + "\t" + Formats::Format(MType()));
    }

    static std::string inFormatter() {
      return(outFormatter() + "%*[^\n]s\n");
    }
  };


  // Specialization 2: Extend specialization 1 with "rest-size" information
  template <bool IsNonStaticChrom, typename MeasureType, bool HasRest>
  struct Bed5<BasicCoords<IsNonStaticChrom, HasRest>, MeasureType, HasRest>
    : public BasicCoords<IsNonStaticChrom, HasRest> {

    // Parameters
    typedef MeasureType MeasurementType;
    static const int NumFields = 5;
    static const bool UseRest = true;

    Bed5() : BaseClass() { *id_ = '\0'; }
    Bed5(const Bed5& c) : BaseClass(c)
      {
        *id_ = '\0';
        static const char* f = (std::string("%s\t") + MFormat).c_str();
        sscanf(rest_, f, id_, &measurement_);
      }
    Bed5(char const* chrom, CoordType start, CoordType end,
         char const* id, MeasureType measurement, char const* rest = NULL)
      : BaseClass(chrom, start, end, rest)
    {
      id_[0] = '\0';
      if ( id && std::strcmp(id, "") != 0 )
        std::strcpy(id_, id);
      measurement_ = measurement;
    }
    explicit Bed5(FILE* inF) : BaseClass()
      { this->readline(inF); }
    explicit Bed5(const std::string& inS) : BaseClass()
      { this->readline(inS); }

    // Properties
    inline void id(char const* id) { *id_ = '\0'; if ( id != NULL ) std::strcpy(id_, id); }
    inline char const* id() const { return id_; }
    inline MeasurementType measurement() const { return measurement_; }
    inline char const* rest() const { return rest_; }

    // IO
    inline void print() const {
      static const std::string lclStatic = outFormatter();
      static char const* format = lclStatic.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline void println() const {
      static const std::string heapFormat = outFormatter() + "\n";
      static char const* format = heapFormat.c_str();
      std::printf(format, chrom_, start_, end_, rest_);
    }
    inline int readline(const std::string& inputLine) {
      *chrom_ = '\0';
      *id_ = '\0';
      *rest_ = '\0';
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      int numScanned = std::sscanf(inputLine.c_str(),
                                   format, chrom_,
                                   &start_, &end_, rest_);
      static const char* f = (std::string("%s\t") + MFormat).c_str();
      sscanf(rest_, f, id_, &measurement_);
      return numScanned + 2;
    }
    inline int readline(FILE* inputFile) {
      *chrom_ = '\0';
      *id_ = '\0';
      *rest_ = '\0';
      static const std::string lclStatic = inFormatter();
      static char const* format = lclStatic.c_str();
      int numScanned = std::fscanf(inputFile,
                                   format, chrom_,
                                   &start_, &end_, rest_);
      static const std::string lclf = (std::string("%s\t") + MFormat);
      static const char* f = lclf.c_str();
      sscanf(rest_, f, id_, &measurement_);
      std::fgetc(inputFile); // Read and discard trailing newline
      return numScanned + 2;
    }

    // Operators
    Bed5& operator=(const Bed5& c) {
      BaseClass::operator=(c);
      static const char* f = (std::string("%s\t") + MFormat).c_str();
      sscanf(rest_, f, id_, &measurement_);
      return *this;
    }
    inline operator MeasurementType() const { return measurement_; }

  protected:
    typedef BasicCoords<IsNonStaticChrom, HasRest> BaseClass;
    using BaseClass::chrom_;
    using BaseClass::start_;
    using BaseClass::end_;
    using BaseClass::rest_;

    typedef typename std::remove_cv<MeasureType>::type MType;
    static constexpr char const* MFormat = Formats::Format(MType());

    char id_[MAXIDSIZE+1];
    MeasureType measurement_;

    static std::string outFormatter() {
      return(BaseClass::outFormatter());
    }

    static std::string inFormatter() {
      return(BaseClass::inFormatter());
    }
  };

} // namespace Bed

#endif // BED_NONEW_HPP
