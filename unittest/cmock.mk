

../test_tools:
	mkdir -p ../test_tools

../test_tools/cmock: | ../test_tools
	mkdir -p ../test_tools/cmock

../test_tools/cmock/download.zip: | ../test_tools/cmock
	@echo "Downloading CMock"
	curl -L "https://github.com/ThrowTheSwitch/CMock/archive/refs/tags/v2.6.0.zip" \
	  -o ../test_tools/cmock/download.zip \
		--fail --show-error --connect-timeout 30

../test_tools/cmock/temp: | ../test_tools/cmock/download.zip
	@echo "Extracting CMock"
	@unzip -q ../test_tools/cmock/download.zip -d ../test_tools/cmock/temp

../test_tools/cmock/src: | ../test_tools/cmock/temp
	@echo "Copying Src CMock Files"
	mv ../test_tools/cmock/temp/*/* ../test_tools/cmock

setup: | ../test_tools/cmock/src

generate_hal_mocks: 
	ruby ../test_tools/cmock/lib/cmock.rb -o mock_config.yaml ../src/hal/gpio.h ../src/hal/timer.h 