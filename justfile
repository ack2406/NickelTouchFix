image := "ghcr.io/pgaskin/nickeltc:1.0"

# Build libnickeltouchfix.so and KoboRoot.tgz with NickelTC in Docker
build:
    docker run --rm -v "{{justfile_directory()}}:/src" -w /src {{image}} sh -c "make clean && make all koboroot"

# Remove build output
clean:
    docker run --rm -v "{{justfile_directory()}}:/src" -w /src {{image}} make clean
