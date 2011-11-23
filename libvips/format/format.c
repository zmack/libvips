/* VIPS function dispatch tables for image format load/save.
 */

/*

    This file is part of VIPS.

    VIPS is free software; you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

 */

/*

    These files are distributed with VIPS - http://www.vips.ecs.soton.ac.uk

 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif /*HAVE_CONFIG_H*/
#include <vips/intl.h>

#include <stdio.h>

#include <vips/vips.h>
#include <vips/internal.h>

/**
 * SECTION: format
 * @short_description: load and save in a variety of formats
 * @stability: Stable
 * @see_also: <link linkend="libvips-image">image</link>
 * @include: vips/vips.h
 *
 * VIPS has a simple system for representing image load and save operations in
 * a generic way.
 *
 * You can ask for a loader for a certain file or select a saver based on a
 * filename. Once you have found a format, you can use it to load a file of
 * that type, save an image to a file of that type, query files for their type
 * and fields, and ask for supported features. You can also call the
 * converters directly, if you like. 
 *
 * If you define a new format, support for
 * it automatically appears in all VIPS user-interfaces. It will also be
 * transparently supported by vips_image_new_from_file() and friends.
 *
 * VIPS comes with VipsFormat for TIFF, JPEG, PNG, Analyze, PPM, OpenEXR, CSV,
 * Matlab, Radiance, RAW, VIPS and one that wraps libMagick. 
 */

/**
 * VipsFormatFlags: 
 * @VIPS_FORMAT_NONE: no flags set
 * @VIPS_FORMAT_PARTIAL: the image may be read lazilly
 * @VIPS_FORMAT_BIGENDIAN: image pixels are most-significant byte first
 *
 * Some hints about the image loader.
 *
 * @VIPS_FORMAT_PARTIAL means that the image can be read directly from the
 * file without needing to be unpacked to a temporary image first. 
 *
 * @VIPS_FORMAT_BIGENDIAN means that image pixels are most-significant byte
 * first. Depending on the native byte order of the host machine, you may
 * need to swap bytes. See copy_swap().
 */

/**
 * VipsFormat:
 *
 * #VipsFormat has these virtual methods:
 *
 * |[
 * typedef struct _VipsFormatClass {
 *   VipsObjectClass parent_class;
 *
 *   gboolean (*is_a)( const char *filename );
 *   int (*header)( const char *filename, IMAGE *out );
 *   int (*load)( const char *filename, IMAGE *out );
 *   int (*save)( IMAGE *in, const char *filename );
 *   VipsFormatFlags (*get_flags)( const char *filename );
 *   int priority;
 *   const char **suffs;
 * } VipsFormatClass;
 * ]|
 *
 * Add a new format to VIPS by subclassing VipsFormat. Subclasses need to 
 * implement at least load() or save(). 
 *
 * These members are:
 *
 * <itemizedlist>
 *   <listitem>
 *     <para>
 * is_a() This function should return %TRUE if the file 
 * contains an image of this type. If you don't define this function, VIPS
 * will use the list of suffixes you supply instead.
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * header() This function should load the image header,
 * but not load any pixel data. If you don't define it, VIPS will use your
 * load() method instead. Return 0 for success, -1 for error, setting
 * vips_error().
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * load() This function should load the image, or perhaps use 
 * vips_image_generate() to
 * attach something to load sections of the image on demand. 
 * Users can embed
 * load options in the filename, see (for example) im_jpeg2vips().
 * If you don't
 * define this method, you can still define save() and have a save-only
 * format.
 * Return 0 for success, -1 for error, setting
 * im_error().
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * save() This function should save the image to the file. 
 * Users can embed
 * save options in the filename, see (for example) im_vips2tiff().
 * If you don't
 * define this method, you can still define load() and have a load-only
 * format.
 * Return 0 for success, -1 for error, setting
 * im_error().
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * get_flags() This function should return a hint about the properties of this
 * loader on this file. If you don't define it, users will always see '0', or
 * no flags. 
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * <structfield>priority</structfield> Where this format should fit in this 
 * list of
 * supported formats. 0 is a sensible value for most formats. Set a negative
 * value if you want to be lower on the list, positive to move up.
 *     </para>
 *   </listitem>
 *   <listitem>
 *     <para>
 * <structfield>suffs</structfield> A %NULL-terminated list of possible file 
 * name
 * suffixes, for example:
 * |[
 * static const char *tiff_suffs[] = { ".tif", ".tiff", NULL };
 * ]|
 * The suffix list is used to select a format to save a file in, and to pick a
 * loader if you don't define is_a().
 *     </para>
 *   </listitem>
 * </itemizedlist>
 *
 * You should also define <structfield>nickname</structfield> and
 * <structfield>description</structfield> in #VipsObject. 
 *
 * At the command-line, use:
 *
 * |[
 * vips --list classes | grep Format
 * ]|
 *
 * To see a list of all the supported formats.
 *
 * For example, the TIFF format is defined like this:
 *
|[
typedef VipsFormat VipsFormatTiff;
typedef VipsFormatClass VipsFormatTiffClass;

static void
vips_format_tiff_class_init( VipsFormatTiffClass *class )
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;
	VipsFormatClass *format_class = (VipsFormatClass *) class;

	object_class->nickname = "tiff";
	object_class->description = _( "TIFF" );

	format_class->is_a = istiff;
	format_class->header = tiff2vips_header;
	format_class->load = im_tiff2vips;
	format_class->save = im_vips2tiff;
	format_class->get_flags = tiff_flags;
	format_class->suffs = tiff_suffs;
}

static void
vips_format_tiff_init( VipsFormatTiff *object )
{
}

G_DEFINE_TYPE( VipsFormatTiff, vips_format_tiff, VIPS_TYPE_FORMAT );
]|
 *
 * Then call vips_format_tiff_get_type() somewhere in your init code to link
 * the format into VIPS (though of course the tiff format is linked in for you
 * already).
 *
 */

/* To iterate over supported formats, we build a temp list of subclasses of 
 * VipsFormat, sort by priority, iterate, and free.
 */

static void *
format_add_class( VipsFormatClass *format, GSList **formats )
{
	/* Append so we don't reverse the list of formats.
	 */
	*formats = g_slist_append( *formats, format );

	return( NULL );
}

static gint
format_compare( VipsFormatClass *a, VipsFormatClass *b )
{
        return( b->priority - a->priority );
}

/**
 * vips_format_map:
 * @base: base class to search below (eg. "VipsFormatLoad")
 * @fn: function to apply to each #VipsFormatClass
 * @a: user data
 * @b: user data
 *
 * Apply a function to every #VipsFormatClass that VIPS knows about. Formats
 * are presented to the function in priority order. 
 *
 * Like all VIPS map functions, if @fn returns %NULL, iteration continues. If
 * it returns non-%NULL, iteration terminates and that value is returned. The
 * map function returns %NULL if all calls return %NULL.
 *
 * See also: vips_slist_map().
 *
 * Returns: the result of iteration
 */
void *
vips_format_map( const char *base, VipsSListMap2Fn fn, void *a, void *b )
{
	GSList *formats;
	void *result;

	formats = NULL;
	(void) vips_class_map_all( g_type_from_name( base ), 
		(VipsClassMapFn) format_add_class, (void *) &formats );

	formats = g_slist_sort( formats, (GCompareFunc) format_compare );
	result = vips_slist_map2( formats, fn, a, b );
	g_slist_free( formats );

	return( result );
}

/* Abstract base class for image formats.
 */

G_DEFINE_ABSTRACT_TYPE( VipsFormat, vips_format, VIPS_TYPE_OBJECT );

static void
vips_format_print_class( VipsObjectClass *object_class, VipsBuf *buf )
{
	VipsFormatClass *class = VIPS_FORMAT_CLASS( object_class );
	const char **p;

	VIPS_OBJECT_CLASS( vips_format_parent_class )->
		print_class( object_class, buf );
	vips_buf_appends( buf, ", " );

	if( class->suffs ) {
		vips_buf_appends( buf, "(" );
		for( p = class->suffs; *p; p++ ) {
			vips_buf_appendf( buf, "%s", *p );
			if( p[1] )
				vips_buf_appends( buf, ", " );
		}
		vips_buf_appends( buf, ") " );
	}

	vips_buf_appends( buf, "priority=%d ", class->priority );

	if( class->is_a )
		vips_buf_appends( buf, "is_a " );
}

static void
vips_format_class_init( VipsFormatClass *class )
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	object_class->nickname = "format";
	object_class->description = _( "file format support" );
	object_class->print_class = vips_format_print_class;
}

static void
vips_format_init( VipsFormat *object )
{
}

/* Abstract base class for image load.
 */

G_DEFINE_ABSTRACT_TYPE( VipsFormatLoad, vips_formats_load, VIPS_TYPE_FORMAT );

static void
vips_format_load_print_class( VipsObjectClass *object_class, VipsBuf *buf )
{
	VipsFormatLoadClass *class = VIPS_FORMAT_LOAD_CLASS( object_class );
	const char **p;

	VIPS_OBJECT_CLASS( vips_format_load_parent_class )->
		print_class( object_class, buf );
	vips_buf_appends( buf, ", " );

	if( class->header )
		vips_buf_appends( buf, "header " );
	if( class->load )
		vips_buf_appends( buf, "load " );
	if( class->get_flags )
		vips_buf_appends( buf, "get_flags " );
}

static void
vips_format_load_class_init( VipsFormatLoadClass *class )
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	object_class->nickname = "formatload";
	object_class->description = _( "format loaders" );
	object_class->print_class = vips_format_load_print_class;
}

static void
vips_format_load_init( VipsFormatLoad *object )
{
}

/* Abstract base class for image savers.
 */

G_DEFINE_ABSTRACT_TYPE( VipsFormatSave, vips_format_save, VIPS_TYPE_FORMAT );

static void
vips_format_print_class( VipsObjectClass *object_class, VipsBuf *buf )
{
	VipsFormatClass *class = VIPS_FORMAT_CLASS( object_class );
	const char **p;

	VIPS_OBJECT_CLASS( vips_format_parent_class )->
		print_class( object_class, buf );
	vips_buf_appends( buf, ", " );

	if( class->suffs ) {
		vips_buf_appends( buf, "(" );
		for( p = class->suffs; *p; p++ ) {
			vips_buf_appendf( buf, "%s", *p );
			if( p[1] )
				vips_buf_appends( buf, ", " );
		}
		vips_buf_appends( buf, ") " );
	}

	if( class->is_a )
		vips_buf_appends( buf, "is_a " );

	if( class->header )
		vips_buf_appends( buf, "header " );
	if( class->load )
		vips_buf_appends( buf, "load " );
	if( class->save )
		vips_buf_appends( buf, "save " );
	if( class->get_flags )
		vips_buf_appends( buf, "get_flags " );
}

static void
vips_format_class_init( VipsFormatClass *class )
{
	VipsObjectClass *object_class = (VipsObjectClass *) class;

	object_class->nickname = "format";
	object_class->description = _( "VIPS file formats" );
	object_class->print_class = vips_format_print_class;
}

static void
vips_format_init( VipsFormat *object )
{
}
