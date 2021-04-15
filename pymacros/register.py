"""Register a codec search function

Code search functions are expected to take one argument, being the encoding name 
in all lower case letters, and return a CodecInfo object. In case a search function 
cannot find a given encoding, it should return None.

Warning:
    Search function registration is not currently reversible, which may cause problems 
    in some cases, such as unit testing or module reloading.
"""  

# External imports
import codecs
import encodings
import io

# Internal imports
import src

def decode(input, errors='strict'):
    text, errors = encodings.utf_8.decode(input, errors)
    return src.wrap.apply(text), errors


class IncrementalDecoder(encodings.utf_8.IncrementalDecoder):
    def decode(self, input, final=False):
        return src.wrap.apply(super().decode(input, final))


class StreamReader(encodings.utf_8.StreamReader):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self.stream = io.StringIO(src.wrap.apply(self.stream.getvalue()))

def search_function(encoding):
    
    """Search function for encoding 'cfgexts'.

    Parameters
    ----------
    encoding : str
        the encoding name.

    Returns
    -------
    CodecInfo
        a CodecInfo object if an encoding exists or None otherwise.
    """

    if encoding != 'cfgexts':
        return None

    return codecs.CodecInfo(
        name='cfgexts',
        encode=encodings.utf_8.encode,
        decode=decode,
        incrementalencoder=encodings.utf_8.IncrementalEncoder,
        incrementaldecoder=IncrementalDecoder,
        streamreader=StreamReader,
        streamwriter=encodings.utf_8.StreamWriter,
    )

# Register cfg-parser as a codec search function

# FIXME: uncomment code when it will be debagged
# codecs.register(search_function)