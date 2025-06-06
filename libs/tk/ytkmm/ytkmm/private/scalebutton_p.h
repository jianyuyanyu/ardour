// -*- c++ -*-
// Generated by gmmproc 2.45.3 -- DO NOT MODIFY!
#ifndef _GTKMM_SCALEBUTTON_P_H
#define _GTKMM_SCALEBUTTON_P_H


#include <ytkmm/private/button_p.h>

#include <glibmm/class.h>

namespace Gtk
{

class ScaleButton_Class : public Glib::Class
{
public:
#ifndef DOXYGEN_SHOULD_SKIP_THIS
  typedef ScaleButton CppObjectType;
  typedef GtkScaleButton BaseObjectType;
  typedef GtkScaleButtonClass BaseClassType;
  typedef Gtk::Button_Class CppClassParent;
  typedef GtkButtonClass BaseClassParent;

  friend class ScaleButton;
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

  const Glib::Class& init();


  static void class_init_function(void* g_class, void* class_data);

  static Glib::ObjectBase* wrap_new(GObject*);

protected:

  //Callbacks (default signal handlers):
  //These will call the *_impl member methods, which will then call the existing default signal callbacks, if any.
  //You could prevent the original default signal handlers being called by overriding the *_impl method.
  static void value_changed_callback(GtkScaleButton* self, gdouble p0);

  //Callbacks (virtual functions):
};


} // namespace Gtk


#endif /* _GTKMM_SCALEBUTTON_P_H */

