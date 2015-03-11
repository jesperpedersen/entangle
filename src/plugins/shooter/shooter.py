#
# Entangle: Tethered Camera Control & Capture
#
# Copyright (C) 2014 Daniel P. Berrange
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.
#

import time

from gi.repository import GObject
from gi.repository import Gtk
from gi.repository import Gdk
from gi.repository import GLib
from gi.repository import Gio
from gi.repository import Peas
from gi.repository import PeasGtk
from gi.repository import Entangle

class ShooterPluginWidget(Gtk.Grid):
    '''The form for controlling parameters of
    the batch mode shooting session and the progress'''


    def do_count_change(self, data):
        self.config.set_shot_count(self.countspin.get_value())

    def do_interval_change(self, data):
        self.config.set_shot_interval(self.intervalspin.get_value())

    def __init__(self, config):
        super(ShooterPluginWidget, self).__init__()

        self.config = config

        self.set_properties(column_spacing=6,
                            row_spacing=6,
                            expand=False)

        self.attach(Gtk.Label("Shot count:"), 0, 0, 1, 1)
        self.countadjust = Gtk.Adjustment(0, 1, 10000, 1, 10, 10);
        self.countspin = Gtk.SpinButton()
        self.countspin.set_adjustment(self.countadjust)
        self.countspin.set_value(self.config.get_shot_count())
        self.countspin.set_properties(expand=True)
        self.countadjust.connect("value-changed", self.do_count_change)
        self.attach(self.countspin, 1, 0, 1, 1)

        self.attach(Gtk.Label("Shot interval:"), 0, 1, 1, 1)
        self.intervaladjust = Gtk.Adjustment(0, 0, 1000, 1, 10, 10);
        self.intervalspin = Gtk.SpinButton()
        self.intervalspin.set_adjustment(self.intervaladjust)
        self.intervalspin.set_value(self.config.get_shot_interval())
        self.intervalspin.set_properties(expand=True)
        self.intervaladjust.connect("value-changed", self.do_interval_change)
        self.attach(self.intervalspin, 1, 1, 1, 1)

        self.show_all()

class ShooterPluginData(GObject.Object):

    __gtype_name__ = "ShooterPluginData"

    def __init__(self, count, interval):
        super(ShooterPluginData, self).__init__()

        self.count = count
        self.interval = interval
        self.automata = None

    def shoot(self):
        self.count = self.count - 1

    def finished(self):
        return self.count == 0

    def wait(self):
        if self.interval == 0:
            # Workaround gphoto bug with camera not ready
            time.sleep(0.2)
        else:
            time.sleep(self.interval)

class ShooterPluginScript(Entangle.ScriptSimple):
    '''The script for controlling the camera'''

    def __init__(self, config):
        super(ShooterPluginScript, self).__init__(
            title="Repeat shooter"
        )

        self.config = config
        self.widget = ShooterPluginWidget(config)

    def do_get_config_widget(self):
        return self.widget

    def do_timeout_callback(self, script_result):
        if script_result.return_error_if_cancelled():
            return

        script_result.automata.capture_async(script_result.get_cancellable(),
                                             self.do_capture_callback,
                                             script_result)

    def do_capture_callback(self, automata, result, script_result):
        try:
            automata.capture_finish(result)
        except GLib.Error as e:
            self.return_task_error(script_result, str(e));
            return

        if script_result.return_error_if_cancelled():
            return

        data = self.get_task_data(script_result)
        data.shoot()
        if data.finished():
            script_result.return_boolean(True)
        else:
            if data.interval > 0:
                GLib.timeout_add_seconds(data.interval,
                                         self.do_timeout_callback,
                                         script_result)
            else:
                automata.capture_async(script_result.get_cancellable(),
                                       self.do_capture_callback,
                                       script_result)

    def do_init_task_data(self):
        return ShooterPluginData(self.config.get_shot_count(),
                                 self.config.get_shot_interval())

    def do_execute(self, automata, cancel, result):
        result.automata = automata
        automata.capture_async(cancel, self.do_capture_callback, result)


class ShooterPluginConfig(Gtk.Grid):
    '''Provides integration with GSettings to read/write
    configuration parameters'''

    def __init__(self, plugin_info):
        Gtk.Grid.__init__(self)
        settingsdir = plugin_info.get_data_dir() + "/schemas"
        sssdef = Gio.SettingsSchemaSource.get_default()
        sss = Gio.SettingsSchemaSource.new_from_directory(settingsdir, sssdef, False)
        schema = sss.lookup("org.entangle-photo.plugins.shooter", False)
        self.settings = Gio.Settings.new_full(schema, None, None)

    def get_shot_count(self):
        return self.settings.get_int("shot-count")

    def set_shot_count(self, count):
        self.settings.set_int("shot-count", count)

    def get_shot_interval(self):
        return self.settings.get_int("shot-interval")

    def set_shot_interval(self, interval):
        self.settings.set_int("shot-interval", interval)


class ShooterPlugin(GObject.Object, Peas.Activatable):
    '''Handles the plugin activate/deactivation and
    tracking of camera manager windows. When a window
    appears, it enables the shooter functionality on
    that window'''
    __gtype_name__ = "ShooterPlugin"

    object = GObject.property(type=GObject.Object)

    def __init__(self):
        GObject.Object.__init__(self)
        self.wins = []
        self.winsigadd = None
        self.winsigrem = None
        self.config = None
        self.script = None

    def do_activate_window(self, win):
        if not isinstance(win, Entangle.CameraManager):
            return

        win.add_script(self.script)
        self.wins.append(win)

    def do_deactivate_window(self, win):
        if not isinstance(win, Entangle.CameraManager):
            return

        win.remove_script(self.script)
        oldwins = self.wins
        self.wins = []
        for w in oldwins:
            if w != win:
                self.wins.append(w)

    def do_activate(self):
        if self.config is None:
            self.config = ShooterPluginConfig(self.plugin_info)
            self.script = ShooterPluginScript(self.config)

        # Windows can be dynamically added/removed so we
        # must track this
        self.winsigadd = self.object.connect(
            "window-added",
            lambda app, win: self.do_activate_window(win))
        self.winsigrem = self.object.connect(
            "window-removed",
            lambda app, win: self.do_deactivate_window(win))

        for win in self.object.get_windows():
            self.do_activate_window(win)

    def do_deactivate(self):
        self.object.disconnect(self.winsigadd)
        self.object.disconnect(self.winsigrem)
        for win in self.object.get_windows():
            self.do_deactivate_window(win)
        self.config = None
        self.script = None
