task :default => [:build, :run, :cleanup]

TARGET = "rd_get_symbols_test"

task :build do
	system("mkdir build") if (!File.exists?(Dir.getwd+"/build"))
	system("clang -DDEBUG -o build/#{TARGET} *.c")
end

task :run do
	system("./build/#{TARGET}")
end

task :cleanup do
	system("rm -Rf ./build")
end
