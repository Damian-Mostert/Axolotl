# Publishing Axolotl

## Quick Start

1. **GitHub Release** (Recommended for open source)
2. **Homebrew** (Mac users)
3. **Chocolatey** (Windows users)
4. **APT/DNF Repository** (Linux users)
5. **Docker Hub** (Containerized)

---

## 1. GitHub Releases

### Prepare Release
```bash
# Tag version
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0

# Create release archives
tar -czf axolotl-v1.0.0-macos.tar.gz src/ include/ examples/ sample/ CMakeLists.txt README.md install.sh uninstall.sh
tar -czf axolotl-v1.0.0-linux.tar.gz src/ include/ examples/ sample/ CMakeLists.txt README.md install.sh uninstall.sh
zip -r axolotl-v1.0.0-windows.zip src/ include/ examples/ sample/ CMakeLists.txt README.md install.bat uninstall.bat
```

### Create Release on GitHub
1. Go to `https://github.com/YOUR_USERNAME/Axolotl/releases/new`
2. Choose tag: `v1.0.0`
3. Title: `Axolotl v1.0.0`
4. Description:
```markdown
## Installation

### macOS/Linux
```bash
curl -fsSL https://github.com/YOUR_USERNAME/Axolotl/raw/main/install.sh | bash
```

### Windows
Download and run `install.bat` as administrator

## What's New
- Initial release
- Full language support
- VS Code extension
- 3D graphics and physics
```
5. Upload: `axolotl-v1.0.0-macos.tar.gz`, `axolotl-v1.0.0-linux.tar.gz`, `axolotl-v1.0.0-windows.zip`
6. Publish release

---

## 2. Homebrew (macOS)

### Create Formula
```bash
# Create homebrew-axolotl repo
mkdir homebrew-axolotl
cd homebrew-axolotl
```

Create `Formula/axolotl.rb`:
```ruby
class Axolotl < Formula
  desc "Axolotl programming language"
  homepage "https://github.com/YOUR_USERNAME/Axolotl"
  url "https://github.com/YOUR_USERNAME/Axolotl/archive/v1.0.0.tar.gz"
  sha256 "YOUR_SHA256_HERE"
  license "MIT"

  depends_on "cmake" => :build
  depends_on "sdl2"
  depends_on "gtk+3"

  def install
    system "cmake", "-S", ".", "-B", "build", *std_cmake_args
    system "cmake", "--build", "build"
    bin.install "build/compiler" => "axolotl"
    share.install "examples", "sample"
    doc.install "README.md"
  end

  test do
    (testpath/"test.axo").write("print(\"Hello\");")
    assert_match "Hello", shell_output("#{bin}/axolotl test.axo")
  end
end
```

### Publish
```bash
git init
git add Formula/axolotl.rb
git commit -m "Add Axolotl formula"
git remote add origin https://github.com/YOUR_USERNAME/homebrew-axolotl.git
git push -u origin main
```

### Users Install
```bash
brew tap YOUR_USERNAME/axolotl
brew install axolotl
```

---

## 3. Chocolatey (Windows)

### Create Package
```powershell
# Install chocolatey packaging tools
choco install chocolatey-core.extension

# Create package structure
mkdir axolotl-choco
cd axolotl-choco
```

Create `axolotl.nuspec`:
```xml
<?xml version="1.0"?>
<package xmlns="http://schemas.microsoft.com/packaging/2015/06/nuspec.xsd">
  <metadata>
    <id>axolotl</id>
    <version>1.0.0</version>
    <title>Axolotl Programming Language</title>
    <authors>YOUR_NAME</authors>
    <projectUrl>https://github.com/YOUR_USERNAME/Axolotl</projectUrl>
    <licenseUrl>https://github.com/YOUR_USERNAME/Axolotl/blob/main/LICENSE</licenseUrl>
    <requireLicenseAcceptance>false</requireLicenseAcceptance>
    <description>Axolotl programming language with 3D graphics support</description>
    <tags>programming-language compiler interpreter</tags>
  </metadata>
  <files>
    <file src="tools\**" target="tools" />
  </files>
</package>
```

Create `tools/chocolateyinstall.ps1`:
```powershell
$ErrorActionPreference = 'Stop'
$packageName = 'axolotl'
$url = 'https://github.com/YOUR_USERNAME/Axolotl/releases/download/v1.0.0/axolotl-v1.0.0-windows.zip'
$installDir = "$(Split-Path -parent $MyInvocation.MyCommand.Definition)"

Install-ChocolateyZipPackage $packageName $url $installDir
```

### Publish
```powershell
choco pack
choco push axolotl.1.0.0.nupkg --source https://push.chocolatey.org/
```

### Users Install
```powershell
choco install axolotl
```

---

## 4. Linux Repositories

### Debian/Ubuntu (APT)

Create `debian/` directory structure:
```bash
mkdir -p debian
```

Create `debian/control`:
```
Source: axolotl
Section: devel
Priority: optional
Maintainer: YOUR_NAME <your@email.com>
Build-Depends: debhelper (>= 10), cmake, g++, libsdl2-dev, libgtk-3-dev
Standards-Version: 4.1.3

Package: axolotl
Architecture: any
Depends: ${shlibs:Depends}, ${misc:Depends}, libsdl2-2.0-0, libgtk-3-0
Description: Axolotl programming language
 A statically-typed programming language with 3D graphics support
```

Build package:
```bash
dpkg-buildpackage -us -uc
```

### Fedora/RHEL (RPM)

Create `axolotl.spec`:
```spec
Name:           axolotl
Version:        1.0.0
Release:        1%{?dist}
Summary:        Axolotl programming language

License:        MIT
URL:            https://github.com/YOUR_USERNAME/Axolotl
Source0:        %{name}-%{version}.tar.gz

BuildRequires:  cmake gcc-c++ SDL2-devel gtk3-devel
Requires:       SDL2 gtk3

%description
Axolotl programming language with 3D graphics support

%prep
%setup -q

%build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build

%install
mkdir -p %{buildroot}%{_bindir}
install -m 755 build/compiler %{buildroot}%{_bindir}/axolotl

%files
%{_bindir}/axolotl
%doc README.md

%changelog
* Mon Jan 01 2024 YOUR_NAME <your@email.com> - 1.0.0-1
- Initial release
```

Build:
```bash
rpmbuild -ba axolotl.spec
```

---

## 5. Docker Hub

Create `Dockerfile`:
```dockerfile
FROM ubuntu:22.04

RUN apt-get update && apt-get install -y \
    cmake g++ libsdl2-dev libgtk-3-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /axolotl
COPY . .

RUN cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && \
    cmake --build build && \
    cp build/compiler /usr/local/bin/axolotl

WORKDIR /workspace
ENTRYPOINT ["axolotl"]
```

Build and publish:
```bash
docker build -t YOUR_USERNAME/axolotl:1.0.0 .
docker tag YOUR_USERNAME/axolotl:1.0.0 YOUR_USERNAME/axolotl:latest
docker push YOUR_USERNAME/axolotl:1.0.0
docker push YOUR_USERNAME/axolotl:latest
```

Users run:
```bash
docker run -v $(pwd):/workspace YOUR_USERNAME/axolotl:latest myfile.axo
```

---

## 6. NPM (for VS Code Extension)

```bash
cd lang-ext
npm install -g vsce
vsce package
vsce publish
```

Users install:
```bash
code --install-extension axolotl-1.0.0.vsix
```

Or publish to marketplace: https://marketplace.visualstudio.com/manage

---

## 7. Website & Documentation

### Create Landing Page
Host on GitHub Pages:
```bash
mkdir docs
cd docs
```

Create `docs/index.html`:
```html
<!DOCTYPE html>
<html>
<head>
    <title>Axolotl Programming Language</title>
    <style>
        body { font-family: Arial; max-width: 800px; margin: 50px auto; }
        .install { background: #f4f4f4; padding: 20px; border-radius: 5px; }
        code { background: #e0e0e0; padding: 2px 5px; }
    </style>
</head>
<body>
    <h1>🦎 Axolotl Programming Language</h1>
    <p>A modern, statically-typed language with built-in 3D graphics</p>
    
    <div class="install">
        <h2>Quick Install</h2>
        <h3>macOS/Linux</h3>
        <code>curl -fsSL https://axolotl-lang.org/install.sh | bash</code>
        
        <h3>Windows</h3>
        <code>choco install axolotl</code>
        
        <h3>Homebrew</h3>
        <code>brew install axolotl</code>
    </div>
    
    <h2>Features</h2>
    <ul>
        <li>Static typing</li>
        <li>3D graphics & physics</li>
        <li>VS Code integration</li>
        <li>Fast compilation</li>
    </ul>
</body>
</html>
```

Enable GitHub Pages in repo settings → Pages → Source: `docs/`

---

## 8. Marketing & Community

### Announce On
- [ ] Reddit: r/programming, r/ProgrammingLanguages
- [ ] Hacker News: https://news.ycombinator.com/submit
- [ ] Dev.to: Write tutorial article
- [ ] Twitter/X: Tweet with #programming
- [ ] Discord: Create community server
- [ ] YouTube: Demo video

### Create
- [ ] Logo and branding
- [ ] Tutorial series
- [ ] Example projects
- [ ] Documentation site
- [ ] Blog posts

---

## Checklist Before Publishing

- [ ] Version number updated everywhere
- [ ] LICENSE file included (MIT recommended)
- [ ] README.md complete with examples
- [ ] CHANGELOG.md created
- [ ] All tests passing
- [ ] Install scripts tested on all platforms
- [ ] VS Code extension packaged
- [ ] GitHub repo public
- [ ] Release notes written
- [ ] Social media accounts created
- [ ] Domain registered (optional)

---

## Recommended First Steps

1. **GitHub Release** - Easiest, immediate
2. **Homebrew** - Mac users love it
3. **Website** - Professional presence
4. **Reddit/HN** - Get initial users
5. **Chocolatey** - Windows distribution
6. **Documentation** - Keep users engaged

Good luck! 🚀
