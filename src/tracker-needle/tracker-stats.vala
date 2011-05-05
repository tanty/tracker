//
// Copyright 2010, Martyn Russell <martyn@lanedo.com>
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
// 02110-1301, USA.
//

using Gtk;

// Added to fix #error for GETTEXT_PACKAGE
private const string b = Config.PACKAGE_NAME;

[DBus (name = "org.freedesktop.Tracker1.Statistics")]
interface Statistics : DBusProxy {
	public abstract string[,] Get () throws DBusError;
}

public class Tracker.Stats : Dialog {
	private Statistics tracker;

	public Stats () {
		this.title = "Statistics";
		this.border_width = 12;
		this.resizable = false;

		setup_dbus ();
		setup_ui ();
	}

	private void setup_dbus () {
		debug ("Setting up statistics D-Bus connection");

		try {
			tracker = GLib.Bus.get_proxy_sync (BusType.SESSION,
			                                   "org.freedesktop.Tracker1",
			                                   "/org/freedesktop/Tracker1/Statistics",
			                                   DBusProxyFlags.DO_NOT_LOAD_PROPERTIES | DBusProxyFlags.DO_NOT_CONNECT_SIGNALS);
		} catch (GLib.IOError e) {
			var msg = new MessageDialog (null,
			                             DialogFlags.MODAL,
			                             MessageType.ERROR,
			                             ButtonsType.CANCEL,
			                             "Error connecting to D-Bus session bus, %s", 
			                             e.message);
			msg.run ();
			Gtk.main_quit ();
		}
	}

	private void setup_ui () {
		debug ("Setting up statistics UI");

		// Spacing between major units
		var vbox = this.get_content_area() as Box;
		vbox.set_spacing (18);
		vbox.set_border_width (0);

		// Label for dialog
		var label = new Label ("The statistics represented here do not reflect their availability, rather the total data stored:");
		label.set_line_wrap (true);
		label.set_alignment (0.0f, 0.5f);
		vbox.pack_start (label, true, true, 0);

		// Size group to line up labels
		var sizegroup = new SizeGroup (Gtk.SizeGroupMode.HORIZONTAL);

		try {
			var result = tracker.Get ();

			for (int i = 0; i < result.length[0]; i++) {
				var key = result[i,0];
				var val = result[i,1];
				string key_used;

				debug ("--> %s = %s", key, val);

				switch (key) {
				case "nao:Tag":
					key_used = ngettext ("Tag", "Tags", val.to_int ());
					break;
				case "nco:Contact":
					key_used = ngettext ("Contact", "Contacts", val.to_int ());
					break;
				case "nfo:Audio":
					key_used = ngettext ("Audio", "Audios", val.to_int ());
					break;
				case "nfo:Document":
					key_used = ngettext ("Document", "Documents", val.to_int ());
					break;
				case "nfo:FileDataObject":
					key_used = ngettext ("File", "Files", val.to_int ());
					break;
				case "nfo:Folder":
					key_used = ngettext ("Folder", "Folders", val.to_int ());
					break;
				case "nfo:Image":
					key_used = ngettext ("Image", "Images", val.to_int ());
					break;
				case "nfo:SoftwareApplication":
					key_used = ngettext ("Application", "Applications", val.to_int ());
					break;
				case "nfo:Video":
			  //case "nmm:Video":
					key_used = ngettext ("Video", "Videos", val.to_int ());
					break;
				case "nmm:MusicAlbum":
					key_used = ngettext ("Album", "Albums", val.to_int ());
					break;
				case "nmm:MusicPiece":
					key_used = ngettext ("Music Track", "Music Tracks", val.to_int ());
					break;
				case "nmm:Photo":
					key_used = ngettext ("Photo", "Photos", val.to_int ());
					break;
				case "nmm:Playlist":
					key_used = ngettext ("Playlist", "Playlists", val.to_int ());
					break;
				case "nmo:Email":
					key_used = ngettext ("Email", "Emails", val.to_int ());
					break;
				
				default:
					continue;
				}

				var hbox = new HBox (false, 12);
				var label_key = new Label (key_used);
				var label_val = new Label (val);

				label_key.set_alignment (0.0f, 0.5f);
				label_val.set_alignment (0.0f, 0.5f);
				hbox.pack_start (label_key, true, true, 0);
				hbox.pack_start (label_val, false, true, 0);

				sizegroup.add_widget (label_key);

				vbox.pack_start (hbox, true, true, 0);
			}
		} catch (DBusError e) {
			warning ("Could not get Tracker statistics: " + e.message);
		}

		// Layout widgets
		vbox.set_spacing (10);

		// Add buttons to button area at the bottom
		add_button (Stock.CLOSE, ResponseType.CLOSE);

		// Connect signals
		this.response.connect (on_response);

		show_all ();
	}

	private void on_response (Dialog source, int response_id) {
		switch (response_id) {
		case ResponseType.CLOSE:
			destroy ();
			break;
		}
	}
}
