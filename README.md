# 🦷 DentaPro — Sistem za upravljanje zubarskom ordinacijom

**Profesionalni C++/Qt5/Qt6 program za zubarsku ordinaciju**

---

## 📋 Funkcije

### 👥 Upravljanje pacijentima
- Dodavanje, uređivanje i brisanje pacijenata
- Podaci: ime, prezime, datum rođenja, spol, telefon, email, adresa, alergije, krvna grupa, napomene
- Brza pretraga po imenu, telefonu ili emailu

### 🦷 Intervencije
- Detaljan zapis svake intervencije:
  - Vrsta intervencije (pregled, vađenje, plomba, devitalizacija, implant, krunica, most, itd.)
  - Broj zuba
  - Dijagnoza i terapija
  - Opis rada
  - Doktor koji je radio
  - Datum i tačno vrijeme
  - Cijena (BAM, EUR, USD, RSD)
  - Status (završeno / zakazano / otkazano)
- **PDF prilog** — priložite digitalni dokument uz svaku intervenciju
- **Elektronski potpis** — pacijent se potpisuje direktno u programu

### 📅 Historija intervencija (Kalendar)
- Kalendar sa označenim danima kada su rađene intervencije
- Kliknite na dan → vidite sve intervencije tog dana
- Detaljan prikaz svake intervencije
- Brz pristup za uređivanje ili brisanje

### 🔍 Napredna pretraga
- Pretraga po:
  - Imenu pacijenta
  - Datumu (od–do)
  - Vrsti intervencije
- Dvaput kliknite na rezultat → otvara kompletan profil pacijenta

### 🖨 Štampanje i izvještaji
- Kompletan izvještaj o pacijentu (sve intervencije, potpisi, PDF reference)
- Print preview prije štampanja
- Profesionalni HTML/CSS dizajn izvještaja
- Uključuje elektronski potpis pacijenta

### 📅 Termini (Appointments)
- Zakazivanje budućih termina
- Pregled nadolazećih termina (30 dana)
- Dashboard sa statistikama

### 📊 Dashboard
- Ukupan broj pacijenata
- Broj intervencija ovog mjeseca
- Prihod ovog mjeseca
- Nadolazeći termini

---

## 🚀 Pokretanje (Windows EXE)

1. **Preuzmite** `DentaPro-Windows-x64.zip` iz GitHub Releases
2. **Raspakujte** u bilo koji folder (npr. `C:\DentaPro\`)
3. **Pokrenite** `DentaPro.exe`

> ℹ️ Nema instalacije — sve je u ZIP fajlu.

### Gdje se čuvaju podaci?
```
%APPDATA%\DentaPro\dentapro.db        ← baza podataka
%APPDATA%\DentaPro\signatures\        ← potpisi pacijenata
```

---

## 🔨 Build iz source koda (za developere)

### Zahtjevi
- Qt 5.15+ ili Qt 6.x
- CMake 3.16+
- C++17 kompajler (MSVC 2019+, GCC 10+, Clang 12+)

### Build na Windowsu
```powershell
git clone https://github.com/VAŠ_USERNAME/DentaPro.git
cd DentaPro
cmake -B build -S . -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
cd build
nmake
```

### Build na Linuxu
```bash
git clone https://github.com/VAŠ_USERNAME/DentaPro.git
cd DentaPro
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
```

---

## 🤖 GitHub Actions — Automatski build

Svaki push na `main` ili `master` branch automatski:
1. Instalira Qt 6.5 i MSVC
2. Kompajlira program
3. Pakuje sve Qt DLL-ove uz EXE
4. Kreira GitHub Release sa ZIP fajlom

### Kako dodati na GitHub:
```bash
git init
git add .
git commit -m "Initial commit — DentaPro v1.0"
git branch -M main
git remote add origin https://github.com/VAŠ_USERNAME/DentaPro.git
git push -u origin main
```

Nakon push-a, idite na **Actions** tab na GitHub-u i pratite build.
EXE se pojavljuje u **Releases** sekciji.

---

## 🛠 Tehnologije

| Komponenta | Tehnologija |
|------------|-------------|
| GUI framework | Qt5/Qt6 Widgets |
| Baza podataka | SQLite (via Qt SQL) |
| Štampanje | Qt PrintSupport |
| Potpis | Custom QWidget |
| Build sistem | CMake |
| CI/CD | GitHub Actions |

---

## 📁 Struktura projekta

```
DentaPro/
├── CMakeLists.txt          ← Build konfiguracija
├── .github/
│   └── workflows/
│       └── build.yml       ← GitHub Actions (Windows EXE)
├── src/
│   ├── main.cpp            ← Entry point
│   ├── models.h            ← Strukture podataka
│   ├── database.h/cpp      ← SQLite manager
│   ├── mainwindow.h/cpp    ← Glavni prozor
│   ├── patientdialog.h/cpp ← Dijalog za pacijente
│   ├── interventiondialog.h/cpp ← Dijalog za intervencije
│   ├── patienthistorywidget.h/cpp ← Kalendar historija
│   ├── signaturewidget.h/cpp ← Potpis pad
│   └── reportgenerator.h/cpp ← Štampanje
└── resources/
    ├── resources.qrc
    └── styles/dental.qss
```

---

*DentaPro — Profesionalni softver za zubarsku ordinaciju*
