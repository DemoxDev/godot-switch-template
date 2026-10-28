extends Node
## Staged 3D feature probe for the Switch port. Every stage prints "STAGE n: applying" before it
## changes the scene and "STAGE n: OK" after 30 presented frames. If the console hangs, the last
## line in sdmc:/switch/<game name>/log.txt names the feature that broke it.

var _world: Node3D
var _cam: Camera3D
var _mesh: MeshInstance3D
var _sun: DirectionalLight3D
var _env: WorldEnvironment
var _layer: CanvasLayer


func _ready() -> void:
	_run.call_deferred()


func _frames(count: int) -> void:
	for i in count:
		await get_tree().process_frame


func _stage(label: String, apply: Callable) -> void:
	print("STAGE %s: applying" % label)
	apply.call()
	await _frames(30)
	print("STAGE %s: OK (fps=%d)" % [label, Engine.get_frames_per_second()])
	await get_tree().create_timer(1.0).timeout


func _run() -> void:
	print("STAGE start: ", RenderingServer.get_video_adapter_name(), " | ", RenderingServer.get_video_adapter_api_version())
	await _stage("0 2D clear colour", func() -> void: RenderingServer.set_default_clear_color(Color(0.2, 0.1, 0.3)))

	await _stage("1 empty 3D + camera", func() -> void:
		_world = Node3D.new()
		add_child(_world)
		_cam = Camera3D.new()
		_world.add_child(_cam)
		_cam.position = Vector3(0, 0.5, 3.5)
		_cam.current = true)

	await _stage("2 unshaded box", func() -> void:
		_mesh = MeshInstance3D.new()
		_mesh.mesh = BoxMesh.new()
		var m := StandardMaterial3D.new()
		m.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
		m.albedo_color = Color.ORANGE
		_mesh.material_override = m
		_world.add_child(_mesh)
		_mesh.rotation = Vector3(0.5, 0.7, 0))

	await _stage("3 lit box + directional light", func() -> void:
		_mesh.material_override = StandardMaterial3D.new()
		(_mesh.material_override as StandardMaterial3D).albedo_color = Color.ORANGE
		_sun = DirectionalLight3D.new()
		_sun.rotation = Vector3(-0.8, 0.6, 0)
		_world.add_child(_sun))

	await _stage("4 + omni light", func() -> void:
		var omni := OmniLight3D.new()
		omni.position = Vector3(1.5, 1.5, 2.0)
		_world.add_child(omni))

	await _stage("5 + normal map", func() -> void:
		var img := Image.create_empty(64, 64, false, Image.FORMAT_RGB8)
		img.fill(Color(0.5, 0.5, 1.0))
		var m := _mesh.material_override as StandardMaterial3D
		m.normal_enabled = true
		m.normal_texture = ImageTexture.create_from_image(img))

	await _stage("6 + sky environment and fog", func() -> void:
		var sky := Sky.new()
		sky.sky_material = ProceduralSkyMaterial.new()
		var env := Environment.new()
		env.background_mode = Environment.BG_SKY
		env.sky = sky
		env.fog_enabled = true
		env.fog_density = 0.02
		_env = WorldEnvironment.new()
		_env.environment = env
		add_child(_env))

	await _stage("7 + custom spatial shader", func() -> void:
		var sh := Shader.new()
		sh.code = "shader_type spatial;\nvoid fragment() { ALBEDO = vec3(UV, 0.5); }"
		var sm := ShaderMaterial.new()
		sm.shader = sh
		_mesh.material_override = sm)

	await _stage("8 + directional shadows", func() -> void:
		_mesh.material_override = StandardMaterial3D.new()
		_sun.shadow_enabled = true)

	await _stage("9 + SubViewport rendering 3D", func() -> void:
		_layer = CanvasLayer.new()
		add_child(_layer)
		var svc := SubViewportContainer.new()
		svc.stretch = true
		svc.size = Vector2(400, 300)
		svc.position = Vector2(20, 20)
		_layer.add_child(svc)
		var sv := SubViewport.new()
		sv.size = Vector2i(400, 300)
		sv.world_3d = get_viewport().world_3d
		svc.add_child(sv)
		var cam2 := Camera3D.new()
		sv.add_child(cam2)
		cam2.global_position = Vector3(2, 1, 3)
		cam2.look_at(Vector3.ZERO)
		cam2.current = true)

	await _stage("10 + glow", func() -> void:
		_env.environment.glow_enabled = true)

	await _stage("11 + screen-texture post shader", func() -> void:
		var rect := ColorRect.new()
		rect.set_anchors_preset(Control.PRESET_FULL_RECT)
		rect.mouse_filter = Control.MOUSE_FILTER_IGNORE
		var sh := Shader.new()
		sh.code = "shader_type canvas_item;\nuniform sampler2D screen : hint_screen_texture, filter_linear;\nvoid fragment() { COLOR = texture(screen, SCREEN_UV) * vec4(1.0, 0.9, 0.9, 1.0); }"
		var sm := ShaderMaterial.new()
		sm.shader = sh
		rect.material = sm
		_layer.add_child(rect))

	print("STAGE done: all stages passed")
