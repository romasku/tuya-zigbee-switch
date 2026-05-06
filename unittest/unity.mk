

../test_tools:
	mkdir -p ../test_tools

../test_tools/unity: | ../test_tools
	mkdir -p ../test_tools/unity

../test_tools/unity/download.zip: | ../test_tools/unity
	@echo "Downloading Unity"
	curl -L "https://github.com/ThrowTheSwitch/Unity/archive/refs/tags/v2.6.1.zip" \
	  -o ../test_tools/unity/download.zip \
		--fail --show-error --connect-timeout 30

../test_tools/unity/temp: | ../test_tools/unity/download.zip
	@echo "Extracting Unity"
	@unzip -q ../test_tools/unity/download.zip -d ../test_tools/unity/temp

../test_tools/unity/src: | ../test_tools/unity/temp
	@echo "Copying Src Unity Files"
	mv ../test_tools/unity/temp/*/* ../test_tools/unity

setup: | ../test_tools/unity/src