// This Source Code Form is licensed MPL-2.0: http://mozilla.org/MPL/2.0
#ifndef __AIDA_VISITOR_HH__
#define __AIDA_VISITOR_HH__

namespace Aida {

// == VisitorDispatcher (element type dispatcher) ==
/// Base template for Visitor classes, dispatches operator() to visit_<type>() methods.
template<class DerivedVisitor>
class VisitorDispatcher {
  ~VisitorDispatcher() {} // private dtor prevents undesirable inheritance
  friend DerivedVisitor;  // allow inheritance for DerivedVisitor only (CRTP)
  DerivedVisitor* derived () { return static_cast<DerivedVisitor*> (this); }
protected:
  typedef const char* Name; ///< Member name argument type for visit() methods.
public:
#ifdef    DOXYGEN
  /// The operator() is used to to visit all of a class's members, it dispatches to type dependent visit_*() methods.
  template<class A> void        operator() (A &a, Name n);
#endif // DOXYGEN

  // dispatch for calls like: visitor (field, "field");

  template<class A,
           REQUIRES< IsBool<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_bool (a, n); }

  template<class A,
           REQUIRES< (std::is_integral<A>::value && !IsBool<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_integral (a, n); }

  template<class A,
           REQUIRES< std::is_floating_point<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_float (a, n); }

  template<class A,
           REQUIRES< std::is_enum<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_enum (a, n); }

  template<class A,
           REQUIRES< (!Has___visit__<A>::value &&
                      DerivesString<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_string (a, n); }

  template<class A,
           REQUIRES< (!Has___visit__<A>::value &&
                      DerivesVector<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_vector (a, n); }

  template<class A,
           REQUIRES< Has___visit__<A>::value > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_visitable (a, n); }

  template<class A,
           REQUIRES< (!Has___visit__<A>::value &&
                      !DerivesString<A>::value &&
                      !DerivesVector<A>::value &&
                      std::is_class<A>::value) > = true> void
  operator() (A &a, Name n)
  { return derived()->visit_class (a, n); }

  // dispatch for calls like: visitor (*this, "property", &set_property, &get_property);

  template<class Klass, class Value,
           REQUIRES< IsBool<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_bool (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (std::is_integral<Value>::value && !IsBool<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_integral (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< std::is_floating_point<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_float (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< std::is_enum<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_enum (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      DerivesString<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_string (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      DerivesVector<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_vector (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value, // Value can be RemoteHandle
           REQUIRES< Has__accept_accessor__<Value, DerivedVisitor>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_accessor_visitable (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value, // Value can be Record
           REQUIRES< Has___visit__<Value>::value > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (const Value&), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_visitable (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }

  template<class Klass, class Value,
           REQUIRES< (!Has__accept_accessor__<Value, DerivedVisitor>::value &&
                      !DerivesString<Value>::value &&
                      !DerivesVector<Value>::value &&
                      std::is_class<Value>::value) > = true> void
  operator() (Klass &instance, Name n, void (Klass::*setter) (Value), Value (Klass::*getter) () const)
  {
    Value v = (instance .* getter) (), tmp = v;
    derived()->visit_class (tmp, n);
    if (tmp != v)
      (instance .* setter) (tmp);
  }
};


// == StdVectorValueHandle ==
/// Special case handling for std::vector<> value references due to std::vector<bool> oddities.
template<class stdvector>
struct StdVectorValueHandle :
  ::std::false_type     /// StdVectorValueHandle<vector<ANYTYPE>>\::value == false
{                       /// is_reference<StdVectorValueHandle<vector<ANYTYPE>>\::type>\::value == true
  typedef typename stdvector::reference type;
};
/// Special case handling for std::vector<bool> elements which provide no bool& references.
template<>
struct StdVectorValueHandle<::std::vector<bool>> :
  ::std::true_type      /// StdVectorValueHandle<vector<bool>>\::value = true
{                       /// is_reference<StdVectorValueHandle<vector<bool>>\::type>\::value == false
  typedef typename ::std::vector<bool>::value_type type;
};


// == FromAnyVisitors ==
/// Visitor to construct a visitable class from an Any::AnyDict.
class FromAnyFieldsVisitor {
  const Any::AnyRec &fields_;
public:
  explicit FromAnyFieldsVisitor (const Any::AnyRec &fields) : fields_ (fields) {}

  template<class A,
           REQUIRES< !Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    for (size_t i = 0; i < fields_.size(); i++)
      if (fields_[i].name == name)
        {
          a = fields_[i].get<A>();
          break;
        }
  }

  template<class A,
           REQUIRES< Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    for (size_t i = 0; i < fields_.size(); i++)
      if (fields_[i].name == name)
        {
          a.__aida_from_any__ (fields_[i]);
          break;
        }
  }
};

/// Visitor to construct an Any::AnySeq from a visitable class.
class FromAnySeqVisitor {
  const Any::AnySeq &anys_;
  size_t             index_;
public:
  explicit FromAnySeqVisitor (const Any::AnySeq &anys) : anys_ (anys), index_ (0) {}

  template<class A,
           REQUIRES< !Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    if (index_ < anys_.size())
      {
        const size_t i = index_++;
        a = anys_[i].get<A>();
      }
  }

  template<class A,
           REQUIRES< Has__aida_from_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    if (index_ < anys_.size())
      {
        const size_t i = index_++;
        a.__aida_from_any__ (anys_[i]);
      }
  }
};


// == ToAnyVisitors ==
/// Visitor to construct an Any::AnyRec from a visitable class.
class ToAnyFieldsVisitor {
  Any::AnyRec &fields_;
public:
  explicit ToAnyFieldsVisitor (Any::AnyRec &fields) : fields_ (fields) {}

  template<class A,
           REQUIRES< !Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    Any::Field f (name, Any (a));
    fields_.push_back (f);
  }

  template<class A,
           REQUIRES< Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    Any::Field f (name, a.__aida_to_any__());
    fields_.push_back (f);
  }
};

/// Visitor to construct an Any::AnySeq from a visitable class.
class ToAnySeqVisitor {
  Any::AnySeq &anys_;
public:
  explicit ToAnySeqVisitor (Any::AnySeq &anys) : anys_ (anys) {}

  template<class A,
           REQUIRES< !Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    anys_.push_back (Any (a));
  }

  template<class A,
           REQUIRES< Has__aida_to_any__<A>::value > = true> void
  operator() (A &a, const char *name)
  {
    anys_.push_back (a.__aida_to_any__());
  }
};


// == Any <-> Visitable ==
template<class Visitable> Any  any_from_visitable (Visitable &visitable);
template<class Visitable> void any_to_visitable   (const Any &any, Visitable &visitable);
template<class Vector>    Any  any_from_sequence  (Vector    &vector);
template<class Vector>    void any_to_sequence    (const Any &any, Vector    &vector);

template<class Visitable> Any
any_from_visitable (Visitable &visitable)
{
  Any::AnyRec fields;
  ToAnyFieldsVisitor visitor (fields);
  static_assert (Has___visit__<Visitable>::value, "Visitable provies __accept__");
  visitable.__accept__ (visitor);
  Any any;
  any.set (fields);
  return any;
}

template<class Visitable> void
any_to_visitable (const Any &any, Visitable &visitable)
{
  const Any::AnyRec &fields = any.get<const Any::AnyRec&>();
  FromAnyFieldsVisitor visitor (fields);
  static_assert (Has___visit__<Visitable>::value, "Visitable provies __accept__");
  visitable.__accept__ (visitor);
}

template<class Vector> Any
any_from_sequence (Vector &vector)
{
  static_assert (DerivesVector<Vector>::value, "Vector derives std::vector");
  Any::AnySeq anys;
  ToAnySeqVisitor visitor (anys);
  for (size_t i = 0; i < vector.size(); i++)
    {
      typedef StdVectorValueHandle< typename ::std::vector<typename Vector::value_type> > VectorValueHandle;
      typename VectorValueHandle::type value_handle = vector[i];
      visitor (value_handle, "element");
      if (VectorValueHandle::value) // copy-by-value
        vector[i] = value_handle;
    }
  Any any;
  any.set (anys);
  return any;
}

template<class Vector> void
any_to_sequence (const Any &any, Vector &vector)
{
  static_assert (DerivesVector<Vector>::value, "Vector derives std::vector");
  const Any::AnySeq &anys = any.get<const Any::AnySeq>();
  FromAnySeqVisitor visitor (anys);
  vector.clear();
  vector.resize (anys.size());
  for (size_t i = 0; i < anys.size(); i++)
    {
      typedef StdVectorValueHandle< typename ::std::vector<typename Vector::value_type> > VectorValueHandle;
      typename VectorValueHandle::type value_handle = vector[i];
      visitor (value_handle, "element");
      if (VectorValueHandle::value) // copy-by-value
        vector[i] = value_handle;
    }
}

} // Aida

#endif // __AIDA_VISITOR_HH__
