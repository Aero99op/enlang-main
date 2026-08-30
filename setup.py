from setuptools import setup, find_packages
import os

with open("README.md", "r", encoding="utf-8") as fh:
    long_description = fh.read()

setup(
    name="enlang",
    version="1.0.7",
    author="Spandan Prayas Patra",
    author_email="spandanpatra1234@gmail.com",
    description="The Universal Natural English Programming Language & Full-Stack Engine",
    long_description=long_description,
    long_description_content_type="text/markdown",
    url="https://github.com/Aero99op/enlang",
    packages=find_packages(),
    include_package_data=True,
    package_data={
        "enlg": ["vscode/*.vsix"],
    },
    classifiers=[
        "Development Status :: 5 - Production/Stable",
        "Intended Audience :: Developers",
        "Programming Language :: Python :: 3",
        "Programming Language :: Python :: 3.8",
        "Programming Language :: Python :: 3.9",
        "Programming Language :: Python :: 3.10",
        "Programming Language :: Python :: 3.11",
        "Programming Language :: Python :: 3.12",
        "Programming Language :: Python :: 3.13",
        "License :: OSI Approved :: MIT License",
        "Operating System :: OS Independent",
        "Topic :: Software Development :: Compilers",
        "Topic :: Software Development :: Interpreters",
        "Topic :: Software Development :: Code Generators",
    ],
    python_requires=">=3.8",
    entry_points={
        "console_scripts": [
            "enlang=enlg.cli:main",
            "enlg=enlg.cli:main",
        ],
    },
)
