/*
 *
 * Copyright (c) 2004
 * Dr John Maddock
 *
 * Use, modification and distribution are subject to the 
 * Boost Software License, Version 1.0. (See accompanying file 
 * LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
 *
 */
 
 /*
  *   LOCATION:    see http://www.boost.org for most recent version.
  *   FILE         cpp_regex_traits.hpp
  *   VERSION      see <boost/version.hpp>
  *   DESCRIPTION: Declares regular expression traits class cpp_regex_traits.
  */

#ifndef BOOST_CPP_REGEX_TRAITS_HPP_INCLUDED
#define BOOST_CPP_REGEX_TRAITS_HPP_INCLUDED

#include <boost/config.hpp>

#ifndef BOOST_NO_STD_LOCALE

#ifndef BOOST_REGEX_TRAITS_DEFAULTS_HPP_INCLUDED
#include <boost/regex/v4/regex_traits_defaults.hpp>
#endif
#ifdef BOOST_HAS_THREADS
#include <boost/regex/static_mutex.hpp>
#endif
#ifndef BOOST_REGEX_PRIMARY_TRANSFORM
#include <boost/regex/v4/primary_transform.hpp>
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_PREFIX
#endif

#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4786)
#endif

namespace boost{ 

//
// forward declaration is needed by some compilers:
//
template <class charT>
class cpp_regex_traits;
   
namespace re_detail{

//
// class parser_buf:
// acts as a stream buffer which wraps around a pair of pointers:
//
template <class charT,
          class traits = ::std::char_traits<charT> >
class parser_buf : public ::std::basic_streambuf<charT, traits>
{
   typedef ::std::basic_streambuf<charT, traits> base_type;
   typedef typename base_type::int_type int_type;
   typedef typename base_type::char_type char_type;
   typedef typename base_type::pos_type pos_type;
   typedef ::std::streamsize streamsize;
   typedef typename base_type::off_type off_type;
public:
   parser_buf() : base_type() { setbuf(0, 0); }
   const charT* getnext() { return this->gptr(); }
protected:
   std::basic_streambuf<charT, traits>* setbuf(char_type* s, streamsize n);
   typename parser_buf<charT, traits>::pos_type seekpos(pos_type sp, ::std::ios_base::openmode which);
   typename parser_buf<charT, traits>::pos_type seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which);
private:
   parser_buf& operator=(const parser_buf&);
   parser_buf(const parser_buf&);
};

template<class charT, class traits>
std::basic_streambuf<charT, traits>*
parser_buf<charT, traits>::setbuf(char_type* s, streamsize n)
{
   this->setg(s, s, s + n);
   return this;
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekoff(off_type off, ::std::ios_base::seekdir way, ::std::ios_base::openmode which)
{
   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   std::ptrdiff_t size = this->egptr() - this->eback();
   std::ptrdiff_t pos = this->gptr() - this->eback();
   charT* g = this->eback();
   switch(way)
   {
   case ::std::ios_base::beg:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + off, g + size);
      break;
   case ::std::ios_base::end:
      if((off < 0) || (off > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + size - off, g + size);
      break;
   case ::std::ios_base::cur:
   {
      std::ptrdiff_t newpos = pos + off;
      if((newpos < 0) || (newpos > size))
         return pos_type(off_type(-1));
      else
         this->setg(g, g + newpos, g + size);
      break;
   }
   default: ;
   }
#ifdef BOOST_MSVC
#pragma warning(push)
#pragma warning(disable:4244)
#endif
   return static_cast<pos_type>(this->gptr() - this->eback());
#ifdef BOOST_MSVC
#pragma warning(pop)
#endif
}

template<class charT, class traits>
typename parser_buf<charT, traits>::pos_type
parser_buf<charT, traits>::seekpos(pos_type sp, ::std::ios_base::openmode which)
{
   if(which & ::std::ios_base::out)
      return pos_type(off_type(-1));
   off_type size = static_cast<off_type>(this->egptr() - this->eback());
   charT* g = this->eback();
   if(off_type(sp) <= size)
   {
      this->setg(g, g + off_type(sp), g + size);
   }
   return pos_type(off_type(-1));
}

//
// class cpp_regex_traits_base:
// acts as a container for locale and the facets we are using.
//
template <class charT>
struct cpp_regex_traits_base
{
   cpp_regex_traits_base(const std::locale& l)
   { imbue(l); }
   std::locale imbue(const std::locale& l);

   std::locale m_locale;
   std::ctype<charT> const* m_pctype;
#ifndef BOOST_NO_STD_MESSAGES
   std::messages<charT> const* m_pmessages;
#endif
   std::collate<charT> const* m_pcollate;
};

template <class charT>
std::locale cpp_regex_traits_base<charT>::imbue(const std::locale& l)
{
   std::locale result(m_locale);
   m_locale = l;
   m_pctype = &BOOST_USE_FACET(std::ctype<charT>, l);
#ifndef BOOST_NO_STD_MESSAGES
   m_pmessages = &BOOST_USE_FACET(std::messages<charT>, l);
#endif
   m_pcollate = &BOOST_USE_FACET(std::collate<charT>, l);
   return result;
}

//
// class cpp_regex_traits_char_layer:
// implements methods that require specialisation for narrow characters:
//
template <class charT>
class cpp_regex_traits_char_layer : public cpp_regex_traits_base<charT>
{
   typedef std::basic_string<charT> string_type;
   typedef std::map<charT, regex_constants::syntax_type> map_type;
   typedef typename map_type::const_iterator map_iterator_type;
public:
   cpp_regex_traits_char_layer(const std::locale& l);

   regex_constants::syntax_type syntax_type(charT c)const
   {
      map_iterator_type i = m_char_map.find(c);
      return ((i == m_char_map.end()) ? 0 : i->second);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      map_iterator_type i = m_char_map.find(c);
      if(i == m_char_map.end())
      {
         if(this->m_pctype->is(std::ctype_base::lower, c)) return regex_constants::escape_type_class;
         if(this->m_pctype->is(std::ctype_base::upper, c)) return regex_constants::escape_type_not_class;
         return 0;
      }
      return i->second;
   }

private:
   string_type get_default_message(regex_constants::syntax_type);
   // TODO: use a hash table when available!
   map_type m_char_map;
};

template <class charT>
cpp_regex_traits_char_layer<charT>::cpp_regex_traits_char_layer(const std::locale& l) 
   : cpp_regex_traits_base<charT>(l)
{
   // we need to start by initialising our syntax map so we know which
   // character is used for which purpose:
#ifndef BOOST_NO_STD_MESSAGES
#ifndef __IBMCPP__
   typename std::messages<charT>::catalog cat = static_cast<std::messages<char>::catalog>(-1);
#else
   typename std::messages<charT>::catalog cat = reinterpret_cast<std::messages<char>::catalog>(-1);
#endif
   std::string cat_name(cpp_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = this->m_pmessages->open(
         cat_name, 
         this->m_locale);
      if((int)cat < 0)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::re_detail::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if((int)cat >= 0)
   {
      try{
         for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
         {
            string_type mss = this->m_pmessages->get(cat, 0, i, get_default_message(i));
            for(typename string_type::size_type j = 0; j < mss.size(); ++j)
            {
               m_char_map[mss[j]] = i;
            }
         }
         this->m_pmessages->close(cat);
      }
      catch(...)
      {
         this->m_pmessages->close(cat);
         throw;
      }
   }
   else
   {
#endif
      for(regex_constants::syntax_type i = 1; i < regex_constants::syntax_max; ++i)
      {
         const char* ptr = get_default_syntax(i);
         while(ptr && *ptr)
         {
            m_char_map[this->m_pctype->widen(*ptr)] = i;
            ++ptr;
         }
      }
#ifndef BOOST_NO_STD_MESSAGES
   }
#endif
}

template <class charT>
typename cpp_regex_traits_char_layer<charT>::string_type 
   cpp_regex_traits_char_layer<charT>::get_default_message(regex_constants::syntax_type i)
{
   const char* ptr = get_default_syntax(i);
   string_type result;
   while(ptr && *ptr)
   {
      result.append(1, this->m_pctype->widen(*ptr));
      ++ptr;
   }
   return result;
}

//
// specialised version for narrow characters:
//
template <>
class BOOST_REGEX_DECL cpp_regex_traits_char_layer<char> : public cpp_regex_traits_base<char>
{
   typedef std::string string_type;
public:
   cpp_regex_traits_char_layer(const std::locale& l)
   : cpp_regex_traits_base<char>(l)
   {
      init();
   }

   regex_constants::syntax_type syntax_type(char c)const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }
   regex_constants::escape_syntax_type escape_syntax_type(char c) const
   {
      return m_char_map[static_cast<unsigned char>(c)];
   }

private:
   regex_constants::syntax_type m_char_map[1u << CHAR_BIT];
   void init();
};

//
// class cpp_regex_traits_implementation:
// provides pimpl implementation for cpp_regex_traits.
//
template <class charT>
class cpp_regex_traits_implementation : public cpp_regex_traits_char_layer<charT>
{
public:
   typedef typename cpp_regex_traits<charT>::char_class_type char_class_type;
   typedef typename std::ctype<charT>::mask                  native_mask_type;
   BOOST_STATIC_CONSTANT(char_class_type, mask_blank = 1u << 16);
   BOOST_STATIC_CONSTANT(char_class_type, mask_word = 1u << 17);
   BOOST_STATIC_CONSTANT(char_class_type, mask_unicode = 1u << 18);
#ifdef __GNUC__
   BOOST_STATIC_CONSTANT(native_mask_type, 
      mask_base = 
         std::ctype<charT>::alnum 
         | std::ctype<charT>::alpha
         | std::ctype<charT>::cntrl
         | std::ctype<charT>::digit
         | std::ctype<charT>::graph
         | std::ctype<charT>::lower
         | std::ctype<charT>::print
         | std::ctype<charT>::punct
         | std::ctype<charT>::space
         | std::ctype<charT>::upper
         | std::ctype<charT>::xdigit);
#else
   BOOST_STATIC_CONSTANT(char_class_type, 
      mask_base = 
         std::ctype<charT>::alnum 
         | std::ctype<charT>::alpha
         | std::ctype<charT>::cntrl
         | std::ctype<charT>::digit
         | std::ctype<charT>::graph
         | std::ctype<charT>::lower
         | std::ctype<charT>::print
         | std::ctype<charT>::punct
         | std::ctype<charT>::space
         | std::ctype<charT>::upper
         | std::ctype<charT>::xdigit);
#endif

   //BOOST_STATIC_ASSERT(0 == (mask_base & (mask_word | mask_unicode)));


   typedef std::basic_string<charT> string_type;
   typedef charT char_type;
   //cpp_regex_traits_implementation();
   cpp_regex_traits_implementation(const std::locale& l);
   std::string error_string(regex_constants::error_type n) const
   {
      if(!m_error_strings.empty())
      {
         std::map<int, std::string>::const_iterator p = m_error_strings.find(n);
         return (p == m_error_strings.end()) ? std::string(get_default_error_string(n)) : p->second;
      }
      return get_default_error_string(n);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      char_class_type result = lookup_classname_imp(p1, p2);
      if(result == 0)
      {
         string_type temp(p1, p2);
         this->m_pctype->tolower(&*temp.begin(), &*temp.begin() + temp.size());
         result = lookup_classname_imp(&*temp.begin(), &*temp.begin() + temp.size());
      }
      return result;
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const;
   string_type transform_primary(const charT* p1, const charT* p2) const;
   string_type transform(const charT* p1, const charT* p2) const
   {
      return this->m_pcollate->transform(p1, p2);
   }
   re_detail::parser_buf<charT>   m_sbuf;            // buffer for parsing numbers.
   std::basic_istream<charT>      m_is;              // stream for parsing numbers.
private:
   std::map<int, std::string>     m_error_strings;   // error messages indexed by numberic ID
   std::map<string_type, char_class_type>  m_custom_class_names; // character class names
   std::map<string_type, string_type>      m_custom_collate_names; // collating element names
   unsigned                       m_collate_type;    // the form of the collation string
   charT                          m_collate_delim;   // the collation group delimiter
   //
   // helpers:
   //
   char_class_type lookup_classname_imp(const charT* p1, const charT* p2) const;
};

#if !defined(BOOST_NO_INCLASS_MEMBER_INITIALIZATION)

template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_blank;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_word;
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_unicode;
#ifdef __GNUC__
template <class charT>
typename cpp_regex_traits_implementation<charT>::native_mask_type const cpp_regex_traits_implementation<charT>::mask_base;
#else
template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type const cpp_regex_traits_implementation<charT>::mask_base;
#endif

#endif

template <class charT>
typename cpp_regex_traits_implementation<charT>::string_type 
   cpp_regex_traits_implementation<charT>::transform_primary(const charT* p1, const charT* p2) const
{
   string_type result;
   //
   // swallowing all exceptions here is a bad idea
   // however at least one std lib will always throw
   // std::bad_alloc for certain arguments...
   //
   try{
      //
      // What we do here depends upon the format of the sort key returned by
      // sort key returned by this->transform:
      //
      switch(m_collate_type)
      {
      case sort_C:
      case sort_unknown:
         // the best we can do is translate to lower case, then get a regular sort key:
         {
            result.assign(p1, p2);
            this->m_pctype->tolower(&*result.begin(), &*result.begin() + result.size());
            result = this->m_pcollate->transform(&*result.begin(), &*result.begin() + result.size());
            break;
         }
      case sort_fixed:
         {
            // get a regular sort key, and then truncate it:
            result.assign(this->m_pcollate->transform(p1, p2));
            result.erase(this->m_collate_delim);
            break;
         }
      case sort_delim:
            // get a regular sort key, and then truncate everything after the delim:
            result.assign(this->m_pcollate->transform(p1, p2));
            std::size_t i;
            for(i = 0; i < result.size(); ++i)
            {
               if(result[i] == m_collate_delim)
                  break;
            }
            result.erase(i);
            break;
      }
   }catch(...){}
   while(result.size() && (charT(0) == *result.rbegin()))
      result.erase(result.size() - 1);
   return result;
}

template <class charT>
typename cpp_regex_traits_implementation<charT>::string_type 
   cpp_regex_traits_implementation<charT>::lookup_collatename(const charT* p1, const charT* p2) const
{
   typedef typename std::map<string_type, string_type>::const_iterator iter_type;
   if(m_custom_collate_names.size())
   {
      iter_type pos = m_custom_collate_names.find(string_type(p1, p2));
      if(pos != m_custom_collate_names.end())
         return pos->second;
   }
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_MSVC, < 1300)\
               && !BOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   std::string name(p1, p2);
#else
   std::string name;
   const charT* p0 = p1;
   while(p0 != p2)
	   name.append(1, char(*p0++));
#endif
   name = lookup_default_collate_name(name);
#if !defined(BOOST_NO_TEMPLATED_ITERATOR_CONSTRUCTORS)\
               && !BOOST_WORKAROUND(BOOST_MSVC, < 1300)\
               && !BOOST_WORKAROUND(__BORLANDC__, <= 0x0551)
   if(name.size())
      return string_type(name.begin(), name.end());
#else
   if(name.size())
   {
	   string_type result;
	   typedef std::string::const_iterator iter;
	   iter b = name.begin();
	   iter e = name.end();
	   while(b != e)
		   result.append(1, charT(*b++));
	   return result;
   }
#endif
   if(p2 - p1 == 1)
      return string_type(1, *p1);
   return string_type();
}

template <class charT>
cpp_regex_traits_implementation<charT>::cpp_regex_traits_implementation(const std::locale& l)
: cpp_regex_traits_char_layer<charT>(l), m_is(&m_sbuf)
{
#ifndef BOOST_NO_STD_MESSAGES
#ifndef __IBMCPP__
   typename std::messages<charT>::catalog cat = static_cast<std::messages<char>::catalog>(-1);
#else
   typename std::messages<charT>::catalog cat = reinterpret_cast<std::messages<char>::catalog>(-1);
#endif
   std::string cat_name(cpp_regex_traits<charT>::get_catalog_name());
   if(cat_name.size())
   {
      cat = this->m_pmessages->open(
         cat_name, 
         this->m_locale);
      if((int)cat < 0)
      {
         std::string m("Unable to open message catalog: ");
         std::runtime_error err(m + cat_name);
         boost::re_detail::raise_runtime_error(err);
      }
   }
   //
   // if we have a valid catalog then load our messages:
   //
   if((int)cat >= 0)
   {
      //
      // Error messages:
      //
      for(boost::regex_constants::error_type i = static_cast<boost::regex_constants::error_type>(0); 
         i <= boost::regex_constants::error_unknown; 
         i = static_cast<boost::regex_constants::error_type>(i + 1))
      {
         const char* p = get_default_error_string(i);
         string_type default_message;
         while(*p)
         {
            default_message.append(1, this->m_pctype->widen(*p));
            ++p;
         }
         string_type s = this->m_pmessages->get(cat, 0, i+200, default_message);
         std::string result;
         for(std::string::size_type j = 0; j < s.size(); ++j)
         {
            result.append(1, this->m_pctype->narrow(s[j], 0));
         }
         m_error_strings[i] = result;
      }
      //
      // Custom class names:
      //
      static const char_class_type masks[] = 
      {
         std::ctype<charT>::alnum,
         std::ctype<charT>::alpha,
         std::ctype<charT>::cntrl,
         std::ctype<charT>::digit,
         std::ctype<charT>::graph,
         std::ctype<charT>::lower,
         std::ctype<charT>::print,
         std::ctype<charT>::punct,
         std::ctype<charT>::space,
         std::ctype<charT>::upper,
         std::ctype<charT>::xdigit,
         cpp_regex_traits_implementation<charT>::mask_blank,
         cpp_regex_traits_implementation<charT>::mask_word,
         cpp_regex_traits_implementation<charT>::mask_unicode,
      };
      static const string_type null_string;
      for(unsigned int j = 0; j <= 13; ++j)
      {
         string_type s(this->m_pmessages->get(cat, 0, j+300, null_string));
         if(s.size())
            this->m_custom_class_names[s] = masks[j];
      }
   }
#endif
   //
   // get the collation format used by m_pcollate:
   //
   m_collate_type = re_detail::find_sort_syntax(this, &m_collate_delim);
}

template <class charT>
typename cpp_regex_traits_implementation<charT>::char_class_type 
   cpp_regex_traits_implementation<charT>::lookup_classname_imp(const charT* p1, const charT* p2) const
{
   static const char_class_type masks[] = 
   {
      0,
      std::ctype<char>::alnum, 
      std::ctype<char>::alpha,
      cpp_regex_traits_implementation<charT>::mask_blank,
      std::ctype<char>::cntrl,
      std::ctype<char>::digit,
      std::ctype<char>::digit,
      std::ctype<char>::graph,
      std::ctype<char>::lower,
      std::ctype<char>::lower,
      std::ctype<char>::print,
      std::ctype<char>::punct,
      std::ctype<char>::space,
      std::ctype<char>::space,
      std::ctype<char>::upper,
      cpp_regex_traits_implementation<charT>::mask_unicode,
      std::ctype<char>::upper,
      std::ctype<char>::alnum | cpp_regex_traits_implementation<charT>::mask_word, 
      std::ctype<char>::alnum | cpp_regex_traits_implementation<charT>::mask_word, 
      std::ctype<char>::xdigit,
   };
   if(m_custom_class_names.size())
   {
      typedef typename std::map<std::basic_string<charT>, char_class_type>::const_iterator map_iter;
      map_iter pos = m_custom_class_names.find(string_type(p1, p2));
      if(pos != m_custom_class_names.end())
         return pos->second;
   }
   std::size_t id = 1 + re_detail::get_default_class_id(p1, p2);
   assert(id < sizeof(masks) / sizeof(masks[0]));
   return masks[id];
}


template <class charT>
boost::shared_ptr<cpp_regex_traits_implementation<charT> > create_cpp_regex_traits(const std::locale& l BOOST_APPEND_EXPLICIT_TEMPLATE_TYPE(charT))
{
   // TODO: create a cache for previously constructed objects.
   return boost::shared_ptr<cpp_regex_traits_implementation<charT> >(new cpp_regex_traits_implementation<charT>(l));
}

} // re_detail

template <class charT>
class cpp_regex_traits
{
private:
   typedef std::ctype<charT>            ctype_type;
public:
   typedef charT                        char_type;
   typedef std::size_t                  size_type;
   typedef std::basic_string<char_type> string_type;
   typedef std::locale                  locale_type;
   typedef boost::uint_least32_t        char_class_type;

   struct boost_extensions_tag{};

   cpp_regex_traits()
      : m_pimpl(re_detail::create_cpp_regex_traits<charT>(std::locale()))
   { }
   static size_type length(const char_type* p)
   {
      return std::char_traits<charT>::length(p);
   }
   regex_constants::syntax_type syntax_type(charT c)const
   {
      return m_pimpl->syntax_type(c);
   }
   regex_constants::escape_syntax_type escape_syntax_type(charT c) const
   {
      return m_pimpl->escape_syntax_type(c);
   }
   charT translate(charT c) const
   {
      return c;
   }
   charT translate_nocase(charT c) const
   {
      return m_pimpl->m_pctype->tolower(c);
   }
   charT translate(charT c, bool icase) const
   {
      return icase ? m_pimpl->m_pctype->tolower(c) : c;
   }
   charT tolower(charT c) const
   {
      return m_pimpl->m_pctype->tolower(c);
   }
   charT toupper(charT c) const
   {
      return m_pimpl->m_pctype->toupper(c);
   }
   string_type transform(const charT* p1, const charT* p2) const
   {
      //
      // swallowing all exceptions here is a bad idea
      // however at least one std lib will always throw
      // std::bad_alloc for certain arguments...
      //
      string_type result;
      try{
         result = m_pimpl->m_pcollate->transform(p1, p2);
         //
         // some implementations append unnecessary trailing \0's:
         while(result.size() && (charT(0) == *result.rbegin()))
            result.erase(result.size() - 1);
      }
      catch(...)
      {
      }
      return result;
   }
   string_type transform_primary(const charT* p1, const charT* p2) const
   {
      return m_pimpl->transform_primary(p1, p2);
   }
   char_class_type lookup_classname(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_classname(p1, p2);
   }
   string_type lookup_collatename(const charT* p1, const charT* p2) const
   {
      return m_pimpl->lookup_collatename(p1, p2);
   }
   bool isctype(charT c, char_class_type f) const
   {
      typedef typename std::ctype<charT>::mask ctype_mask;
      if((f & re_detail::cpp_regex_traits_implementation<charT>::mask_base) 
         && (m_pimpl->m_pctype->is(
            static_cast<ctype_mask>(f & re_detail::cpp_regex_traits_implementation<charT>::mask_base), c)))
         return true;
      else if((f & re_detail::cpp_regex_traits_implementation<charT>::mask_unicode) && re_detail::is_extended(c))
         return true;
      else if((f & re_detail::cpp_regex_traits_implementation<charT>::mask_word) && (c == '_'))
         return true;
      else if((f & re_detail::cpp_regex_traits_implementation<charT>::mask_blank) 
         && m_pimpl->m_pctype->is(std::ctype<charT>::space, c)
         && !re_detail::is_separator(c))
         return true;
      return false;
   }
   int toi(const charT*& p1, const charT* p2, int radix)const;
   int value(charT c, int radix)const
   {
      const charT* pc = &c;
      return toi(pc, pc + 1, radix);
   }
   locale_type imbue(locale_type l)
   {
      std::locale result(getloc());
      m_pimpl = re_detail::create_cpp_regex_traits<charT>(l);
      return result;
   }
   locale_type getloc()const
   {
      return m_pimpl->m_locale;
   }
   std::string error_string(regex_constants::error_type n) const
   {
      return m_pimpl->error_string(n);
   }

   //
   // extension:
   // set the name of the message catalog in use (defaults to "boost_regex").
   //
   static std::string catalog_name(const std::string& name);
   static std::string get_catalog_name();

private:
   boost::shared_ptr<re_detail::cpp_regex_traits_implementation<charT> > m_pimpl;
   //
   // catalog name handler:
   //
   static std::string& get_catalog_name_inst();

#ifdef BOOST_HAS_THREADS
   static static_mutex& get_mutex_inst();
#endif
};


template <class charT>
int cpp_regex_traits<charT>::toi(const charT*& first, const charT* last, int radix)const
{
   m_pimpl->m_sbuf.pubsetbuf(const_cast<charT*>(static_cast<const charT*>(first)), static_cast<std::streamsize>(last-first));
   m_pimpl->m_is.clear();
   if(std::abs(radix) == 16) m_pimpl->m_is >> std::hex;
   else if(std::abs(radix) == 8) m_pimpl->m_is >> std::oct;
   else m_pimpl->m_is >> std::dec;
   int val;
   if(m_pimpl->m_is >> val)
   {
      first = first + ((last - first) - m_pimpl->m_sbuf.in_avail());
      return val;
   }
   else
      return -1;
}

template <class charT>
std::string cpp_regex_traits<charT>::catalog_name(const std::string& name)
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   get_catalog_name_inst() = name;
   return result;
}

template <class charT>
std::string& cpp_regex_traits<charT>::get_catalog_name_inst()
{
   static std::string s_name;
   return s_name;
}

template <class charT>
std::string cpp_regex_traits<charT>::get_catalog_name()
{
#ifdef BOOST_HAS_THREADS
   static_mutex::scoped_lock lk(get_mutex_inst());
#endif
   std::string result(get_catalog_name_inst());
   return result;
}

#ifdef BOOST_HAS_THREADS
template <class charT>
static_mutex& cpp_regex_traits<charT>::get_mutex_inst()
{
   static static_mutex s_mutex = BOOST_STATIC_MUTEX_INIT;
   return s_mutex;
}
#endif


} // boost

#ifdef BOOST_MSVC
#pragma warning(pop)
#endif

#ifdef BOOST_HAS_ABI_HEADERS
#  include BOOST_ABI_SUFFIX
#endif

#endif

#endif
