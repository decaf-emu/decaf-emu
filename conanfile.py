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

    while True:
        sys.stdout.write(question + prompt)
        choice = raw_input().lower()
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
      'accept_defaults': [True, False],
   }
   default_options = {
      'accept_defaults': False,
      'ffmpeg:shared': True,
   }

   def requirements(self):
      dependency_list = [
         ('ffmpeg/4.0.2@bincrafters/stable', 'yes'),
         ('libcurl/7.61.1@bincrafters/stable', 'yes'),
         ('OpenSSL/1.0.2n@conan/stable', 'yes'),
         ('sdl2/2.0.9@bincrafters/stable', 'yes'),
         ('zlib/1.2.11@conan/stable','yes')
      ]

      print 'Default enabled dependencies:'
      for dependency, default in dependency_list:
         if default == 'yes':
            print "%s" % (dependency)
      print ''

      print 'Default disabled dependencies:'
      for dependency, default in dependency_list:
         if default == 'no':
            print "%s" % (dependency)
      print ''

      accept_defaults = self.options.accept_defaults
      if not accept_defaults:
         accept_defaults = not query_yes_no('Customise which dependencies to acquire from Conan?', default='no')

      for dependency, default in dependency_list:
         if accept_defaults or query_yes_no(dependency, default=default):
            self.requires(dependency)
