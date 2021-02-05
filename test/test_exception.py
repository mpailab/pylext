import cfgexts.parser.wrap as parser 

if __name__ == "__main__":

    try:
        parser.loadFile("myfilename")
    except RuntimeError as err:
        print("RuntimeError: " + str(err))