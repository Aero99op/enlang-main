"""Test Suite for Universal Enlang Script (.enlgs) Capabilities.

Validates TypeScript shapes, OOP blueprints, async/generators, declarative UI components,
functional array pipelines, full-stack HTTP servers, state stores, 3D world DSL, and destructuring.
"""

import unittest
from enlgs.compiler import compile_enlgs_source

class TestUniversalEnlgsSyntax(unittest.TestCase):

    def test_typescript_shapes(self):
        source = """
in script:
    shape Developer:
        name is text
        experienceYears is number
        skills is list of text
        isRemote is boolean
"""
        js = compile_enlgs_source(source)
        self.assertIn("@typedef {Object} Developer", js)
        self.assertIn("@property {text} name", js)

    def test_oop_blueprints(self):
        source = """
in script:
    blueprint Entity3D:
        to initialize with posX, posY, posZ:
            this.x = posX
            this.y = posY
            this.z = posZ

        to do translate with dx, dy:
            this.x = this.x + dx
            this.y = this.y + dy

    blueprint CelestialBody extends Entity3D:
        to initialize with posX, posY, posZ, mass:
            super with posX, posY, posZ
            this.mass = mass
"""
        js = compile_enlgs_source(source)
        self.assertIn("class Entity3D {", js)
        self.assertIn("constructor(posX, posY, posZ) {", js)
        self.assertIn("class CelestialBody extends Entity3D {", js)
        self.assertIn("super(posX, posY, posZ);", js)

    def test_async_and_generators(self):
        source = """
in script:
    async to do fetchProfile with userId:
        create response as await fetch("https://api.example.com/" + userId)
        create data as await response.json()
        return data

    generator to do countSequence with maxVal:
        create i as 0
        while i < maxVal:
            yield i
            i = i + 1
"""
        js = compile_enlgs_source(source)
        self.assertIn("async function fetchProfile(userId) {", js)
        self.assertIn("function* countSequence(maxVal) {", js)
        self.assertIn("yield i;", js)

    def test_destructuring_and_spread(self):
        source = """
in script:
    create user as { name: "Prayas", role: "Architect" }
    extract name, role from user
    create extended as [...user.skills, "Three.js"]
"""
        js = compile_enlgs_source(source)
        self.assertIn("const { name, role } = user;", js)

    def test_functional_array_pipelines(self):
        source = """
in script:
    create numbers as [1, 2, 3, 4, 5, 6]
    create evens as filter numbers where item % 2 == 0
    create doubled as map evens using item: item * 2
    create firstBig as find in doubled where item > 5
"""
        js = compile_enlgs_source(source)
        self.assertIn("numbers.filter(item => item % 2 == 0)", js)
        self.assertIn("evens.map(item => item * 2)", js)
        self.assertIn("doubled.find(item => item > 5)", js)

    def test_declarative_ui_components(self):
        source = """
in script:
    component MetricCard with title, value:
        make element "div" with class "card":
            add element "h4" with text title
            add element "p" with text value
"""
        js = compile_enlgs_source(source)
        self.assertIn("function MetricCard({ title, value } = {}) {", js)
        self.assertIn("document.createElement('div')", js)
        self.assertIn("document.createElement('h4')", js)

    def test_fullstack_http_server(self):
        source = """
in script:
    serve http on port 8080:
        route get "/api/health":
            return json { status: "UP" }
        route post "/api/data":
            return json { success: true }
"""
        js = compile_enlgs_source(source)
        self.assertIn("http.createServer", js)
        self.assertIn("method === 'GET' && url === '/api/health'", js)
        self.assertIn("server.listen(8080", js)

    def test_centralized_state_store(self):
        source = """
in script:
    store AppStore:
        state count as 0
        state theme as "dark"

        to do increment:
            state.count = state.count + 1
"""
        js = compile_enlgs_source(source)
        self.assertIn("const AppStore = (function() {", js)
        self.assertIn("getState: () => ({ ...state })", js)
        self.assertIn("increment: function() {", js)

    def test_3d_world_dsl(self):
        source = """
in script:
    world 3d on "webgl-canvas":
        create scene as new THREE.Scene()
        rotate sunMesh by y 0.005, x 0.002
        on every animation frame:
            rotate sunMesh by y 0.001
"""
        js = compile_enlgs_source(source)
        self.assertIn("sunMesh.rotation.y += 0.005;", js)
        self.assertIn("sunMesh.rotation.x += 0.002;", js)
        self.assertIn("function _enlgs_animLoop() {", js)

    def test_websockets(self):
        source = """
in script:
    connect websocket to "wss://api.example.com" myWs
    when myWs receives:
        show "Received:", data
"""
        js = compile_enlgs_source(source)
        self.assertIn("new WebSocket('wss://api.example.com')", js)
        self.assertIn("myWs.addEventListener('message'", js)

if __name__ == "__main__":
    unittest.main()
