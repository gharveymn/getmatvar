import glob

def replace():
	for filename in glob.iglob("./" + '**/*.txt', recursive=True):
		if filename.endswith(".txt"):
			file = open(filename, "r")
			text = (file.read()).replace(",", "")
			file.close()
			file = open(filename,"w")
			file.write(text)
			file.close()


if __name__ == "__main__":
	replace()
