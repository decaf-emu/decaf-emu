from conans import ConanFile
import sys

# Thanks stackoverflow: https://stackoverflow.com/a/3041990
def query_yes_no(question, default="yes"):
    """Ask a yes/no question via raw_input() and return their answer.

    "question" is a string that is presented to the user.
    "default" is the presumed answer if the user just hits <Enter>.
        It must be "yes" (the default), "no" or None (meaning
        an answer is required of the user).

    The "answer" return value is True for "yes" or False for "no".
    """
    valid = {"yes": True, "y": True, "ye": True,
             "no": False, "n": False}
    if default is None:
        prompt = " [y/n] "
    elif default == "yes":
        prompt = " [Y/n] "
    elif default == "no":
        prompt = " [y/N] "
    else:
        raise ValueError("invalid default answer: '%s'" % default)

    global input
    try: input = raw_input
    except NameError: pass

    while True:
        sys.stdout.write(question + prompt)
        choice = input().lower()
        if default is not None and choice == '':
            return valid[default]
        elif choice in valid:
            return valid[choice]
        else:
            sys.stdout.write("Please respond with 'yes' or 'no' "
                             "(or 'y' or 'n').\n")

class DecafConan(ConanFile):
   generators = "cmake"
   settings = 'os'
   options = {
      'silent': [True, False],
      'ffmpeg': [True, False],
      'curl': [True, False],
      'openssl': [True, False],
      'sdl2': [True, False],
      'zlib': [True, False],
   }
   default_options = {
      'silent': False,
      'ffmpeg': True,
      'curl': True,
      'openssl': True,
      'sdl2': True,
      'zlib': True,

      'ffmpeg:shared': True,
   }

   def requirements(self):
      dependency_list = [
         ('ffmpeg/4.0.2@bincrafters/stable', 'yes' if self.options.ffmpeg else 'no'),
         ('libcurl/7.61.1@bincrafters/stable', 'yes' if self.options.curl else 'no'),
         ('OpenSSL/1.0.2n@conan/stable', 'yes' if self.options.openssl else 'no'),
         ('sdl2/2.0.9@bincrafters/stable', 'yes' if self.options.sdl2 else 'no'),
         ('zlib/1.2.11@conan/stable', 'yes' if self.options.zlib else 'no')
      ]

      print('Enabled dependencies:')
      for dependency, enabled in dependency_list:
         if enabled == 'yes':
            print("%s" % (dependency))
      print('')

      print('Disabled dependencies:')
      for dependency, enabled in dependency_list:
         if enabled == 'no':
            print("%s" % (dependency))
      print('')

      silent = self.options.silent
      if not silent:
         silent = not query_yes_no('Customise which dependencies to acquire from Conan?', default='no')

      for dependency, enabled in dependency_list:
         if (silent and enabled == 'yes') or \
            (not silent and query_yes_no(dependency, default=enabled)):
            self.requires(dependency)
