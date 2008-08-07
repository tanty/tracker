/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*- */
/* 
 * Copyright (C) 2008, Nokia (urho.konttori@nokia.com)
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA  02110-1301, USA. 
 */  

#include <time.h>
#include <string.h>

#include <glib.h>
#include <glib-object.h>
#include <glib/gtestutils.h>

#include <libtracker-common/tracker-type-utils.h>

#include <tracker-test-helpers.h>

static void
test_date_format ()
{
        gchar *result;

        result = tracker_date_format ("");
        g_assert (result == NULL);


        /* Fails 
        result = tracker_date_format ("1978"); //Audio.ReleaseDate
        g_assert (tracker_test_helpers_cmpstr_equal (result, "1978-01-01T00:00:00"));
        */

        result = tracker_date_format ("2008-06-14");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "2008-06-14T00:00:00"));
        g_free (result);

        result = tracker_date_format ("20080614000000");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "2008-06-14T00:00:00"));
        g_free (result);

        result = tracker_date_format ("20080614000000Z");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "2008-06-14T00:00:00Z"));
        g_free (result);

        result = tracker_date_format ("Mon Jun 14 04:20:20 2008"); //MS Office
        g_assert (tracker_test_helpers_cmpstr_equal (result, "2008-06-14T04:20:20"));
        g_free (result);

        result = tracker_date_format ("2008:06:14 04:20:20"); //Exif style
        g_assert (tracker_test_helpers_cmpstr_equal (result, "2008-06-14T04:20:20"));
        g_free (result);

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                result = tracker_date_format (NULL);
        }
        g_test_trap_assert_failed ();

}

static void
test_string_to_date ()
{
	GDate     *expected;
	GDate     *result;
	time_t     result_time_t;
        gchar     *input = "2008-06-16T11:10:10+0600";

	expected = g_date_new_dmy (16, G_DATE_JUNE, 2008);
	
        result_time_t = tracker_string_to_date (input);

	result = g_date_new ();
	g_date_set_time_t (result, result_time_t);

        g_assert_cmpint (g_date_get_year (expected), ==, g_date_get_year (result));
        g_assert_cmpint (g_date_get_day (expected), ==, g_date_get_day (result));
        g_assert_cmpint (g_date_get_month (expected), ==, g_date_get_month (result));

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                result_time_t = tracker_string_to_date (NULL);
        }
        g_test_trap_assert_failed ();

        result_time_t = tracker_string_to_date ("");
        g_assert_cmpint (result_time_t, ==, -1);

        result_time_t = tracker_string_to_date ("i am not a date");
        g_assert_cmpint (result_time_t, ==, -1);
        
        /* Fails! Check the code
        result_time_t = tracker_string_to_date ("2008-06-32T04:23:10+0000");
        g_assert_cmpint (result_time_t, ==, -1);
        */
}

static void
test_date_to_string ()
{
        struct tm *original;
        time_t     input;
        gchar     *result;

        original = g_new0 (struct tm, 1);
        original->tm_sec = 10;
        original->tm_min = 53;
        original->tm_hour = 23;
        original->tm_mday = 16;
        original->tm_mon = 5;
        original->tm_year = 108;
        original->tm_isdst = 1;
        
        input = mktime (original);

        result = tracker_date_to_string (input);

        g_assert (result != NULL && strncmp (result, "2008-06-16T23:53:10", 19) == 0);
}


static void
test_long_to_string ()
{
        glong n;
        gchar *result;

        n = 10050;
        result = tracker_long_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "10050"));
        g_free (result);

        n = -9950;
        result = tracker_long_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "-9950"));
        g_free (result);
}

static void
test_int_to_string ()
{
        gint n;
        gchar *result;

        n = 654;
        result = tracker_int_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "654"));
        g_free (result);

        n = -963;
        result = tracker_int_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "-963"));
        g_free (result);

}


static void
test_uint_to_string ()
{
        guint n;
        gchar *result;

        n = 100;
        result = tracker_uint_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "100"));
        g_free (result);
}

static void
test_gint32_to_string ()
{
        gint32 n;
        gchar *result;

        n = 100;
        result = tracker_gint32_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "100"));
        g_free (result);

        n = -96;
        result = tracker_gint32_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "-96"));
        g_free (result);

}


static void
test_guint32_to_string ()
{
        guint32 n;
        gchar *result;

        n = 100;
        result = tracker_guint32_to_string (n);
        g_assert (tracker_test_helpers_cmpstr_equal (result, "100"));
        g_free (result);

}


static void
test_string_to_uint ()
{
        guint num_result, rc;

        rc = tracker_string_to_uint ("10", &num_result);
        
        g_assert (rc);
        g_assert_cmpint (num_result, ==, 10);


        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                rc = tracker_string_to_uint (NULL, &num_result);
        }
        g_test_trap_assert_failed ();

        // ???? FIXME
        rc = tracker_string_to_uint ("-20", &num_result);
        // ????

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                tracker_string_to_uint (NULL, &num_result);
        }
        g_test_trap_assert_failed ();

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                tracker_string_to_uint ("199", NULL);
        }
        g_test_trap_assert_failed ();

        rc = tracker_string_to_uint ("i am not a number", &num_result);
        g_assert (!rc);
        g_assert_cmpint (rc, ==, 0);
}


static void
test_string_in_string_list ()
{
        gchar *complete = "This is an extract of text with different terms an props like Audio:Title ...";
        gchar **pieces;

        pieces = g_strsplit (complete, " ", -1);

        g_assert_cmpint (tracker_string_in_string_list ("is", pieces), ==, 1);
        g_assert_cmpint (tracker_string_in_string_list ("Audio:Title", pieces), ==, 12);

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                g_assert_cmpint (tracker_string_in_string_list (NULL, pieces), ==, -1);
        }
        g_test_trap_assert_failed ();

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                g_assert_cmpint (tracker_string_in_string_list ("terms", NULL), ==, -1);
        }
        g_test_trap_assert_failed ();
}

static void
test_gslist_to_string_list () 
{
        GSList *input = NULL;
        gchar **result;

        input = g_slist_prepend (input, "four");
        input = g_slist_prepend (input, "three");
        input = g_slist_prepend (input, "two");
        input = g_slist_prepend (input, "one");

        result = tracker_gslist_to_string_list (input);

        g_assert (tracker_test_helpers_cmpstr_equal (result[0], "one")
                  && tracker_test_helpers_cmpstr_equal (result[1], "two")
                  && tracker_test_helpers_cmpstr_equal (result[2], "three")
                  && tracker_test_helpers_cmpstr_equal (result[3], "four"));

        g_strfreev (result);

        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                result = tracker_gslist_to_string_list (NULL);
        }
        g_test_trap_assert_failed ();
}


static void
test_string_list_to_string () 
{
        gchar *input = "one two three four";
        gchar **pieces;
        gchar *result;

        pieces = g_strsplit (input, " ", 4);
        
        result = tracker_string_list_to_string (pieces, 4, ' ');
        g_assert (tracker_test_helpers_cmpstr_equal (input, result));
        g_free (result);

        result = tracker_string_list_to_string (pieces, 3, '_');
        g_assert (tracker_test_helpers_cmpstr_equal ("one_two_three", result));
        g_free (result);


        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                result = tracker_string_list_to_string (NULL, 6, 'x');
        }
        g_test_trap_assert_failed ();

        result = tracker_string_list_to_string (pieces, -1, ' ');
        g_assert (tracker_test_helpers_cmpstr_equal (input, result));
        g_free (result);

        result = tracker_string_list_to_string (pieces, 6, ' ');
        g_assert (tracker_test_helpers_cmpstr_equal (input, result));
        g_free (result);

        g_strfreev (pieces);
}


static void
test_boolean_as_text_to_number ()
{
        gchar *result;

        /* Correct true values */
        result = tracker_boolean_as_text_to_number ("True");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "1"));
        g_free (result);


        result = tracker_boolean_as_text_to_number ("TRUE");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "1"));
        g_free (result);

        result = tracker_boolean_as_text_to_number ("true");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "1"));
        g_free (result);

        /* Correct false values */
        result = tracker_boolean_as_text_to_number ("False");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "0"));
        g_free (result);

        result = tracker_boolean_as_text_to_number ("FALSE");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "0"));
        g_free (result);

        result = tracker_boolean_as_text_to_number ("false");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "0"));
        g_free (result);

        /* Invalid values */
        result = tracker_boolean_as_text_to_number ("Thrue");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "Thrue"));
        g_free (result);

        result = tracker_boolean_as_text_to_number ("Falsez");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "Falsez"));
        g_free (result);

        result = tracker_boolean_as_text_to_number ("Other invalid value");
        g_assert (tracker_test_helpers_cmpstr_equal (result, "Other invalid value"));
        g_free (result);

        
        if (g_test_trap_fork (0, G_TEST_TRAP_SILENCE_STDERR)) {
                result = tracker_boolean_as_text_to_number (NULL);
        }
        g_test_trap_assert_failed ();
}


int
main (int argc, char **argv) {

        int result;

	g_type_init ();
	g_test_init (&argc, &argv, NULL);

        g_test_add_func ("/libtracker-common/tracker-type-utils/boolean_as_text_to_number",
                         test_boolean_as_text_to_number);
        g_test_add_func ("/libtracker-common/tracker-type-utils/string_list_as_list",
                         test_string_list_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/gslist_to_string_list",
                         test_gslist_to_string_list);
        g_test_add_func ("/libtracker-common/tracker-type-utils/string_in_string_list",
                         test_string_in_string_list);
        g_test_add_func ("/libtracker-common/tracker-type-utils/string_to_uint",
                         test_string_to_uint);
        g_test_add_func ("/libtracker-common/tracker-type-utils/guint32_to_string",
                         test_guint32_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/gint32_to_string",
                         test_gint32_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/uint_to_string",
                         test_uint_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/int_to_string",
                         test_int_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/long_to_string",
                         test_long_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/date_format",
                         test_date_format);
        g_test_add_func ("/libtracker-common/tracker-type-utils/date_to_string",
                         test_date_to_string);
        g_test_add_func ("/libtracker-common/tracker-type-utils/string_to_date",
                         test_string_to_date);
        result = g_test_run ();
        
        return result;
}
