# Koala

Koala is an experimental project designed to explore new ideas and possibilities involving the JavaScript engine, application environments, and runtime development.

It is an experimental application environment and runtime for creating and deploying platforms built around JavaScript.

It is sandboxed by default and allows JavaScript applications to be distributed as a single, simple file based on the ZIP format. The goal is to make it possible to build applications, games, platforms, and other kinds of software with JavaScript and distribute them in a portable way.

The idea behind Koala originally came from an experiment to create this kind of environment on Android. The project is now designed to be portable, allowing different systems and platforms to build their own environments around JavaScript.

## Application Bundles

Koala applications are distributed as a single ZIP-based bundle.

A bundle must contain `Payload` and `Resources`. `Manifest.conf` is an optional but recommended convention for storing application metadata and configuration.

```text
Application.jsa
├── Payload/
│   └── main.js
├── Resources/
│   ├── icon.png
│   └── data.txt
└── Manifest.conf
```

### Payload

`Payload` contains the JavaScript code and other files that belong to the application itself.

The application entrypoint is located inside this directory, but it does not need to be defined by a manifest. The host environment can provide the entrypoint directly.

### Resources

`Resources` contains files that can be accessed by the application through the Koala API.

These can include images, text files, binary data, or other assets used by the application.

### Manifest.conf

`Manifest.conf` is an optional and recommended convention for storing application metadata and configuration.

Koala does not require a specific manifest format. The recommended `Manifest.conf` format is provided as a convenient way to make application bundles more modular and portable, while allowing platforms to define their own fields and rules.

For example:

```ini
name = My Application
package = com.someone.app
author = Someone
version = 1.0.0
entrypoint = main.js
```

The `entrypoint` can also be provided directly by the host environment instead of being stored in the manifest.

Platforms can define their own metadata, fields, and rules without requiring changes to the Koala runtime.

## Runtime

Koala provides the runtime required to execute JavaScript applications and acts as a bridge between JavaScript and the host platform.

The runtime is designed to be portable, allowing different platforms to build their own environments around JavaScript while keeping the application format and JavaScript environment consistent.

## Building

Koala uses Forge as its build system.

It uses QuickJS-NG as its JavaScript engine and Miniz for handling ZIP-based application bundles.

The licenses for these projects are included with their respective source code inside `Sources`.

## License

Koala is licensed under the BSD 3-Clause License. See [license.txt](license.txt) for the full license text.