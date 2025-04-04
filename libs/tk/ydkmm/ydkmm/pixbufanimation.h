// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GDKMM_PIXBUFANIMATION_H
#define _GDKMM_PIXBUFANIMATION_H


#include <glibmm/ustring.h>
#include <sigc++/sigc++.h>

/* $Id: pixbufanimation.hg,v 1.1 2003/01/21 13:38:37 murrayc Exp $ */

/* box.h
 *
 * Copyright (C) 1998-2002 The gtkmm Development Team
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

#include <glibmm/object.h>
#include <ydkmm/pixbuf.h>
#include <ydkmm/pixbufanimationiter.h>
#include <ydk-pixbuf/ydk-pixbuf.h>


#ifndef DOXYGEN_SHOULD_SKIP_THIS
typedef struct _GdkPixbufAnimation GdkPixbufAnimation;
typedef struct _GdkPixbufAnimationClass GdkPixbufAnimationClass;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */


#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace Gdk
{ class PixbufAnimation_Class; } // namespace Gdk
#endif //DOXYGEN_SHOULD_SKIP_THIS

namespace Gdk
{

/** The gdk-pixbuf library provides a simple mechanism to load and represent animations. 
 * An animation is conceptually a series of frames to be displayed over time. 
 * Each frame is the same size. The animation may not be represented as a series of frames internally; 
 * for example, it may be stored as a sprite and instructions for moving the sprite around a background. 
 * To display an animation you don't need to understand its representation, however; you just ask 
 * gdk-pixbuf what should be displayed at a given point in time.
 */

class PixbufAnimation : public Glib::Object
{
  
#ifndef DOXYGEN_SHOULD_SKIP_THIS

public:
  typedef PixbufAnimation CppObjectType;
  typedef PixbufAnimation_Class CppClassType;
  typedef GdkPixbufAnimation BaseObjectType;
  typedef GdkPixbufAnimationClass BaseClassType;

private:  friend class PixbufAnimation_Class;
  static CppClassType pixbufanimation_class_;

private:
  // noncopyable
  PixbufAnimation(const PixbufAnimation&);
  PixbufAnimation& operator=(const PixbufAnimation&);

protected:
  explicit PixbufAnimation(const Glib::ConstructParams& construct_params);
  explicit PixbufAnimation(GdkPixbufAnimation* castitem);

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

public:
  virtual ~PixbufAnimation();

  /** Get the GType for this class, for use with the underlying GObject type system.
   */
  static GType get_type()      G_GNUC_CONST;

#ifndef DOXYGEN_SHOULD_SKIP_THIS


  static GType get_base_type() G_GNUC_CONST;
#endif

  ///Provides access to the underlying C GObject.
  GdkPixbufAnimation*       gobj()       { return reinterpret_cast<GdkPixbufAnimation*>(gobject_); }

  ///Provides access to the underlying C GObject.
  const GdkPixbufAnimation* gobj() const { return reinterpret_cast<GdkPixbufAnimation*>(gobject_); }

  ///Provides access to the underlying C instance. The caller is responsible for unrefing it. Use when directly setting fields in structs.
  GdkPixbufAnimation* gobj_copy();

private:

  //gtkmmproc error: gdk_pixbuf_animation_ref : ignored method defs lookup failed//gtkmmproc error: gdk_pixbuf_animation_unref : ignored method defs lookup failed
protected:

public:

  static Glib::RefPtr<PixbufAnimation> create_from_file(const Glib::ustring& filename);

  
  int get_width() const;
  
  int get_height() const;
  
  bool is_static_image() const;
  
  Glib::RefPtr<Pixbuf> get_static_image();
  
  Glib::RefPtr<PixbufAnimationIter> get_iter(const GTimeVal* start_time);


public:

public:
  //C++ methods used to invoke GTK+ virtual functions:

protected:
  //GTK+ Virtual Functions (override these to change behaviour):

  //Default Signal Handlers::


};

} /* namespace Gdk */


namespace Glib
{
  /** A Glib::wrap() method for this object.
   * 
   * @param object The C instance.
   * @param take_copy False if the result should take ownership of the C instance. True if it should take a new copy or ref.
   * @result A C++ instance that wraps this C instance.
   *
   * @relates Gdk::PixbufAnimation
   */
  Glib::RefPtr<Gdk::PixbufAnimation> wrap(GdkPixbufAnimation* object, bool take_copy = false);
}


#endif /* _GDKMM_PIXBUFANIMATION_H */

