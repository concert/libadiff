from __future__ import print_function  # Just to shut my editor up
from asyncio import get_event_loop
from functools import wraps
import sys
import argh

from terminal import Terminal, Keyboard


def state_change(func):
    wraps(func)
    def wrapped(self, *args, **kwargs):
        result = func(self, *args, **kwargs)
        self.draw()
        return result
    return wrapped


class DiffApp:
    def __init__(self, filename_a, filename_b):
        self.filename_a = filename_a
        self.filename_b = filename_b

        # App state:
        self.playing = True
        # Bool to track which file we're actually listening to:
        self.a_active = True

        self.terminal = Terminal()
        self.keyboard = Keyboard()
        self.bindings = {
            'q': lambda: self._do_exit('Quit normally'),
            'space': self._do_toggle_play}
        self.loop = get_event_loop()

        # Draw state:
        self._drawn = False

    @staticmethod
    def _do_exit(message):
        sys.exit(message)

    @state_change
    def _do_toggle_play(self):
        self.playing = not self.playing

    def _read_stdin(self):
        key = self.keyboard[self.terminal.infile.read()]
        action = self.bindings.get(key, lambda: None)
        action()

    def _make_file_label(self, filename, active):
        label_width = max(len(self.filename_a), len(self.filename_b))
        label = filename.rjust(label_width)
        if active:
            return '[{}]'.format(label)
        else:
            return ' {} '.format(label)

    @staticmethod
    def _make_time_label(seconds):
        # FIXME: need to pad the mins on the max time we're dealing with
        return '{:d}:{:02d}'.format(*divmod(seconds, 60))

    def draw(self):
        if self._drawn:
            print(self.terminal.move_up * 3, end='')
        label_a = self._make_file_label(self.filename_a, self.a_active)
        time_a = self._make_time_label(128)
        mock_diff_a = '-' * (
            self.terminal.width - len(label_a) - len(time_a) - 2)
        print(label_a, mock_diff_a, time_a)
        play_char = '>' if self.playing else ' '
        print(play_char.center(len(label_a)))
        label_b = self._make_file_label(self.filename_b, not self.a_active)
        time_b = self._make_time_label(143)
        mock_diff_b = '-' * (
            self.terminal.width - len(label_b) - len(time_b) - 2)
        print(label_b, mock_diff_b, time_b, end='')
        self.terminal.stream.flush()
        self._drawn = True

    def run(self):
        if self.terminal.infile.isatty():
            self.loop.add_reader(self.terminal.infile, self._read_stdin)
        with self.terminal.unbuffered_input(), (
                self.terminal.nonblocking_input()), (
                self.terminal.hidden_cursor()):
            self.draw()
            try:
                self.loop.run_forever()
            except KeyboardInterrupt:
                self._do_exit('Keyboard interrupt')


def diff(filename_a, filename_b):
    '''Mock app that diffs two audio streams and provides a UI to the user to
    explore that diff.'''
    DiffApp(filename_a, filename_b).run()


if __name__ == '__main__':
    argh.dispatch_command(diff)
