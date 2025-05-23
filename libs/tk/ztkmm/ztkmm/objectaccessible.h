// -*- c++ -*-
// Generated by gtkmmproc -- DO NOT MODIFY!
#ifndef _ATKMM_OBJECTACCESSIBLE_H
#define _ATKMM_OBJECTACCESSIBLE_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: objectaccessible.hg,v 1.4 2006/04/12 11:11:24 murrayc Exp $ */

/* Copyright (C) 1998-2002 The gtkmm Development Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
 
#include <ztkmm/object.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _AtkGObjectAccessible AtkGObjectAccessible;
typedef struct _AtkGObjectAccessibleClass AtkGObjectAccessibleClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


namespace Atk
{ class ObjectAccessible_Class; } // namespace Atk
namespace Atk
{

/** This object class is derived from AtkObject and can be used as a basis implementing accessible objects.
 * This can be used as a basis for implementing accessible objects for Glib::Objects which are not derived from
 * Gtk::Widget. One example of its use is in providing an accessible object for GnomeCanvasItem in the GAIL library.
 */

class ObjectAccessible : public Atk::Object
{
   
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef ObjectAccessible CppObjectType;
  typedef ObjectAccessible_Class CppClassType;
  typedef AtkGObjectAccessible BaseObjectType;
  typedef AtkGObjectAccessibleClass BaseClassType;

private:  friend class ObjectAccessible_Class;
  static CppClassType objectaccessible_class_;

private:
  // noncopyable
  ObjectAccessible(const ObjectAccessible&);
  ObjectAccessible& operator=(const ObjectAccessible&);

protected:
  explicit ObjectAccessible(const Glib::ConstructParams& construct_params);
  explicit ObjectAccessible(AtkGObjectAccessible* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~ObjectAccessible();

  /** Get the GType for this class, for use with the underlying GObject type system.
   */
  static GType get_type()      G_GNUC_CONST;

#ifndef DOXYGEN_SHOULD_SKIP_THIS


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  AtkGObjectAccessible*       gobj()       { return reinterpret_cast<AtkGObjectAccessible*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const AtkGObjectAccessible* gobj() const { return reinterpret_cast<AtkGObjectAccessible*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  AtkGObjectAccessible* gobj_copy();

private:

protected:

  
  /** Gets the GObject for which @a obj is the accessible object.
   * @return A Object which is the object for which @a obj is the accessible object.
   */
  Glib::RefPtr<Glib::Object> get_object();
  
  /** Gets the GObject for which @a obj is the accessible object.
   * @return A Object which is the object for which @a obj is the accessible object.
   */
  Glib::RefPtr<const Glib::Object> get_object() const;

  
  /** Gets the accessible object for the specified @a obj.
   * @param obj A Object.
   * @return A Atk::Object which is the accessible object for the @a obj.
   */
  static Glib::RefPtr<Atk::Object> for_object(const Glib::RefPtr<Glib::Object>& obj);
  
  /** Gets the accessible object for the specified @a obj.
   * @param obj A Object.
   * @return A Atk::Object which is the accessible object for the @a obj.
   */
  static Glib::RefPtr<const Atk::Object> for_object(const Glib::RefPtr<const Glib::Object>& obj);


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


};

} // namespace Atk


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Atk::ObjectAccessible
   */
  Glib::RefPtr<Atk::ObjectAccessible> wrap(AtkGObjectAccessible* object, bool take_copy = false);
}


#endif /* _ATKMM_OBJECTACCESSIBLE_H */

