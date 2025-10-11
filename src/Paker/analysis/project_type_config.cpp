#include "Paker/analysis/project_type_config.h"
#include <map>
#include <vector>
#include <string>

namespace Paker {
namespace Analysis {

ProjectTypeConfig::ProjectTypeConfig() {
    initialize_project_indicators();
    initialize_performance_indicators();
    initialize_security_indicators();
    initialize_testing_indicators();
    initialize_ml_features();
    initialize_code_quality_indicators();
    initialize_architecture_patterns();
}

void ProjectTypeConfig::initialize_project_indicators() {
    // Web应用关键词（高精度版）
    project_indicators_["web_application"] = {
        // 核心Web协议
        "http", "https", "http/2", "http/3", "quic", "websocket", "sse", "server_sent_events",
        "rest", "restful", "api", "endpoint", "resource", "crud", "create", "read", "update", "delete",
        
        // Web框架
        "boost-beast", "crow", "cpp-httplib", "pistache", "cpprest", "drogon", "oatpp", "seastar",
        "cppcms", "wttr", "cpp-httplib", "httplib", "cpprestsdk", "beast", "asio", "boost_asio",
        
        // 服务器技术
        "nginx", "apache", "lighttpd", "caddy", "traefik", "envoy", "istio", "linkerd",
        "fastcgi", "cgi", "wsgi", "asgi", "uwsgi", "gunicorn", "unicorn", "puma",
        
        // 微服务架构
        "microservice", "microservices", "service_mesh", "api_gateway", "gateway", "load_balancer",
        "proxy", "reverse_proxy", "sidecar", "circuit_breaker", "bulkhead", "timeout",
        
        // 序列化格式
        "json", "xml", "yaml", "toml", "ini", "csv", "tsv", "protobuf", "msgpack", "avro",
        "thrift", "flatbuffers", "capnproto", "bond", "cereal", "boost_serialization",
        
        // 数据库技术
        "mysql", "postgresql", "sqlite", "mongodb", "redis", "cassandra", "elasticsearch",
        "influxdb", "timescaledb", "clickhouse", "druid", "kafka", "pulsar", "rabbitmq",
        
        // 认证授权
        "oauth", "oauth2", "openid", "openid_connect", "saml", "jwt", "jws", "jwe",
        "session", "cookie", "authentication", "authorization", "rbac", "abac", "acl",
        
        // 容器化部署
        "docker", "kubernetes", "container", "orchestration", "helm", "kustomize", "operator",
        "pod", "deployment", "service", "ingress", "configmap", "secret", "persistent_volume"
    };
    
    // 后端服务关键词（新增）
    project_indicators_["backend_service"] = {
        // 服务架构
        "backend", "backend_service", "api_service", "microservice", "service", "daemon",
        "background_service", "worker", "consumer", "producer", "queue_worker", "cron_job",
        
        // 数据存储
        "database", "db", "sql", "nosql", "key_value", "document", "graph", "time_series",
        "mysql", "postgresql", "sqlite", "mongodb", "redis", "cassandra", "elasticsearch",
        "influxdb", "timescaledb", "clickhouse", "druid", "neo4j", "arangodb", "orientdb",
        
        // 消息队列
        "message_queue", "mq", "queue", "broker", "kafka", "pulsar", "rabbitmq", "activemq",
        "zeromq", "nanomsg", "nats", "redis_streams", "amqp", "mqtt", "stomp", "jms",
        
        // 缓存技术
        "cache", "caching", "redis", "memcached", "hazelcast", "ignite", "caffeine",
        "ehcache", "guava_cache", "lru", "lfu", "ttl", "expiration", "eviction",
        
        // 搜索技术
        "search", "search_engine", "lucene", "elasticsearch", "solr", "sphinx", "xapian",
        "full_text_search", "inverted_index", "tf_idf", "bm25", "relevance", "ranking",
        
        // 监控指标
        "monitoring", "metrics", "telemetry", "observability", "logging", "tracing",
        "prometheus", "grafana", "jaeger", "zipkin", "opentelemetry", "datadog", "newrelic",
        
        // 配置管理
        "configuration", "config", "settings", "environment", "env", "properties", "yaml",
        "json", "toml", "ini", "consul", "etcd", "zookeeper", "apollo", "nacos"
    };
    
    // 前端应用关键词（新增）
    project_indicators_["frontend_application"] = {
        // 前端框架
        "react", "vue", "angular", "svelte", "ember", "backbone", "knockout", "jquery",
        "bootstrap", "tailwind", "bulma", "foundation", "semantic_ui", "material_ui",
        
        // 构建工具
        "webpack", "rollup", "parcel", "vite", "esbuild", "swc", "babel", "typescript",
        "sass", "less", "stylus", "postcss", "autoprefixer", "css_modules", "styled_components",
        
        // 状态管理
        "redux", "mobx", "vuex", "zustand", "recoil", "jotai", "valtio", "effector",
        "state_management", "store", "action", "reducer", "selector", "middleware",
        
        // 路由
        "react_router", "vue_router", "angular_router", "reach_router", "wouter",
        "routing", "navigation", "history", "location", "params", "query", "hash",
        
        // 测试
        "jest", "mocha", "chai", "enzyme", "testing_library", "cypress", "playwright",
        "puppeteer", "selenium", "webdriver", "karma", "jasmine", "vitest", "ava"
    };
    
    // 桌面应用关键词（高精度版）
    project_indicators_["desktop_application"] = {
        // GUI框架
        "qt", "gtk", "wxwidgets", "fltk", "imgui", "dear_imgui", "nuklear", "nanogui",
        "qwidget", "qapplication", "qmainwindow", "qdialog", "qpushbutton", "qlabel",
        "gtkmm", "wxframe", "wxpanel", "wxbutton", "wxstatictext", "cef", "electron",
        
        // 窗口系统
        "window", "dialog", "widget", "control", "component", "layout", "menu", "toolbar",
        "statusbar", "menubar", "context_menu", "popup", "modal", "non_modal", "tab",
        "splitter", "scrollbar", "progressbar", "slider", "checkbox", "radiobutton",
        
        // 事件处理
        "event", "signal", "slot", "callback", "handler", "listener", "observer",
        "mouse_event", "keyboard_event", "focus_event", "resize_event", "paint_event",
        
        // 跨平台
        "cross_platform", "portable", "native", "desktop", "application", "standalone",
        "installer", "package", "bundle", "executable", "binary", "distribution",
        
        // 图形渲染
        "opengl", "directx", "vulkan", "metal", "canvas", "painter", "drawing", "gdi",
        "gdi+", "cairo", "skia", "freetype", "harfbuzz", "pango", "fontconfig"
    };
    
    // 移动应用关键词（新增）
    project_indicators_["mobile_application"] = {
        // 移动平台
        "android", "ios", "mobile", "smartphone", "tablet", "wearable", "iot", "embedded",
        "react_native", "flutter", "xamarin", "ionic", "cordova", "phonegap", "capacitor",
        
        // 移动框架
        "android_studio", "xcode", "swift", "kotlin", "java", "objective_c", "dart",
        "android_sdk", "ios_sdk", "ndk", "jni", "swift_ui", "jetpack_compose", "flutter",
        
        // 移动UI
        "material_design", "cupertino", "human_interface_guidelines", "responsive",
        "adaptive", "touch", "gesture", "swipe", "pinch", "zoom", "scroll", "navigation",
        
        // 移动特性
        "camera", "gps", "accelerometer", "gyroscope", "magnetometer", "proximity",
        "light_sensor", "orientation", "vibration", "notification", "push_notification",
        "background_task", "foreground_service", "background_service", "wakelock",
        
        // 移动存储
        "sqlite", "realm", "room", "core_data", "shared_preferences", "user_defaults",
        "keychain", "secure_storage", "encrypted_storage", "file_system", "cache",
        
        // 移动网络
        "http", "https", "rest", "api", "websocket", "socket_io", "grpc", "graphql",
        "offline", "sync", "caching", "retry", "timeout", "connection_pool"
    };
    
    // 游戏应用关键词（新增）
    project_indicators_["game_application"] = {
        // 游戏引擎
        "unity", "unreal", "godot", "cryengine", "lumberyard", "source", "id_tech",
        "frostbite", "cryengine", "lumberyard", "source", "id_tech", "frostbite",
        "ogre", "irrlicht", "panda3d", "horde3d", "bgfx", "magnum", "three_js",
        
        // 游戏框架
        "sdl", "sfml", "allegro", "cocos2d", "phaser", "pixi", "three_js", "babylon",
        "playcanvas", "construct", "game_maker", "rpg_maker", "scratch", "blockly",
        
        // 图形渲染
        "opengl", "vulkan", "directx", "metal", "webgl", "gles", "glsl", "hlsl",
        "shader", "vertex", "fragment", "compute", "geometry", "tessellation",
        "raytracing", "pathtracing", "global_illumination", "pbr", "hdr", "tone_mapping",
        
        // 物理引擎
        "physics", "physics_engine", "bullet", "box2d", "chipmunk", "havok", "physx",
        "newton", "ode", "reactphysics3d", "jolt", "rapier", "bevy", "rapier3d",
        "collision_detection", "collision_response", "rigid_body", "soft_body", "fluid",
        
        // 音频系统
        "audio", "sound", "music", "3d_audio", "spatial_audio", "fmod", "wwise",
        "openal", "alut", "portaudio", "alsa", "pulseaudio", "directsound", "xaudio2",
        
        // 输入系统
        "input", "keyboard", "mouse", "joystick", "gamepad", "touch", "gesture",
        "vr", "ar", "oculus", "vive", "hololens", "leap_motion", "kinect", "leap",
        
        // 游戏逻辑
        "game_loop", "update", "render", "fixed_timestep", "delta_time", "fps",
        "state_machine", "behavior_tree", "ai", "pathfinding", "steering", "flocking"
    };
    
    // 嵌入式系统关键词（高精度版）
    project_indicators_["embedded_system"] = {
        // 实时操作系统
        "freertos", "zephyr", "threadx", "rtos", "vxworks", "qnx", "integrity",
        "xTaskCreate", "xTaskDelete", "xQueueCreate", "xSemaphoreCreate", "mbed_os",
        "contiki", "riot", "nuttx", "chibios", "ecos", "ucos", "rtems", "embox",
        
        // 微控制器
        "stm32", "arduino", "esp32", "esp8266", "nrf52", "pic", "avr", "arm_cortex",
        "mcu", "microcontroller", "microprocessor", "soc", "fpga", "cpld", "asic",
        "arm_m0", "arm_m3", "arm_m4", "arm_m7", "arm_a7", "arm_a9", "arm_a53",
        
        // 硬件抽象
        "hal", "driver", "bsp", "board_support", "hardware_abstraction", "hsm",
        "gpio", "uart", "spi", "i2c", "pwm", "adc", "dac", "timer", "interrupt",
        "dma", "watchdog", "rtc", "crystal", "oscillator", "pll", "clock", "reset",
        
        // 嵌入式特性
        "bare_metal", "bootloader", "firmware", "embedded", "iot", "sensor", "actuator",
        "real_time", "low_power", "energy_efficient", "battery", "power_management",
        "sleep_mode", "deep_sleep", "hibernate", "wake_up", "power_gating", "clock_gating",
        
        // 通信协议
        "modbus", "can", "lin", "flexray", "ethernet", "wifi", "bluetooth", "zigbee",
        "lora", "nb_iot", "cellular", "gprs", "lte", "5g", "nfc", "rfid", "ir", "usb",
        "rs232", "rs485", "tcp_ip", "udp", "mqtt", "coap", "websocket", "http"
    };
    
    // IoT设备关键词（新增）
    project_indicators_["iot_device"] = {
        // IoT平台
        "iot", "internet_of_things", "smart_device", "connected_device", "edge_device",
        "aws_iot", "azure_iot", "google_cloud_iot", "ibm_watson_iot", "thingworx",
        "particle", "arduino_iot", "raspberry_pi", "beaglebone", "orange_pi",
        
        // 传感器
        "sensor", "actuator", "temperature", "humidity", "pressure", "light", "motion",
        "accelerometer", "gyroscope", "magnetometer", "proximity", "ultrasonic",
        "infrared", "camera", "microphone", "speaker", "display", "led", "buzzer",
        
        // 通信协议
        "mqtt", "coap", "http", "websocket", "tcp", "udp", "wifi", "bluetooth", "zigbee",
        "lora", "nb_iot", "cellular", "ethernet", "usb", "serial", "i2c", "spi", "uart",
        
        // 数据处理
        "data_processing", "analytics", "machine_learning", "ai", "edge_computing",
        "streaming", "batch_processing", "real_time", "latency", "throughput",
        "data_compression", "encryption", "security", "privacy", "anonymization",
        
        // 云集成
        "cloud", "aws", "azure", "gcp", "firebase", "supabase", "heroku", "vercel",
        "serverless", "lambda", "functions", "api_gateway", "load_balancer", "cdn"
    };
    
    // 数据科学关键词（新增）
    project_indicators_["data_science"] = {
        // 数据处理
        "data_science", "data_analysis", "data_processing", "data_mining", "big_data",
        "pandas", "numpy", "scipy", "matplotlib", "seaborn", "plotly", "bokeh",
        "dask", "vaex", "polars", "arrow", "parquet", "hdf5", "netcdf", "zarr",
        
        // 机器学习
        "machine_learning", "ml", "scikit_learn", "sklearn", "tensorflow", "pytorch",
        "keras", "xgboost", "lightgbm", "catboost", "mlpack", "shark", "dlib",
        "opencv", "pillow", "scikit_image", "mahotas", "simpleitk", "itk",
        
        // 深度学习
        "deep_learning", "neural_network", "cnn", "rnn", "lstm", "gru", "transformer",
        "attention", "bert", "gpt", "t5", "roberta", "albert", "xlnet", "distilbert",
        "resnet", "vgg", "alexnet", "inception", "densenet", "mobilenet", "efficientnet",
        
        // 统计学习
        "statistics", "statistical_learning", "regression", "classification", "clustering",
        "dimensionality_reduction", "pca", "ica", "lda", "tsne", "umap", "manifold_learning",
        "bayesian", "mcmc", "variational_inference", "gaussian_process", "kernel_methods",
        
        // 可视化
        "visualization", "plotting", "charts", "graphs", "dashboards", "interactive",
        "matplotlib", "seaborn", "plotly", "bokeh", "altair", "ggplot", "d3", "observable",
        "tableau", "power_bi", "grafana", "kibana", "superset", "metabase"
    };
    
    // 游戏引擎关键词（高精度版）
    project_indicators_["game_engine"] = {
        // 图形API
        "opengl", "vulkan", "directx", "metal", "webgl", "gles", "glsl", "hlsl",
        "shader", "vertex", "fragment", "compute", "geometry", "tessellation",
        "raytracing", "pathtracing", "global_illumination", "pbr", "hdr", "tone_mapping",
        
        // 游戏框架
        "sdl", "sfml", "allegro", "cocos2d", "unity", "unreal", "godot", "cryengine",
        "ogre", "irrlicht", "panda3d", "horde3d", "bgfx", "magnum", "three_js",
        "phaser", "pixi", "babylon", "playcanvas", "construct", "game_maker",
        
        // 图形概念
        "rendering", "graphics", "texture", "mesh", "sprite", "animation", "skeleton",
        "rigging", "blending", "lighting", "shadow", "reflection", "refraction",
        "post_processing", "gamma_correction", "anti_aliasing", "mipmapping", "lod",
        
        // 物理引擎
        "physics", "collision", "detection", "response", "rigid_body", "soft_body",
        "fluid", "particle", "cloth", "hair", "bullet", "box2d", "chipmunk", "havok",
        "physx", "newton", "ode", "reactphysics3d", "jolt", "rapier", "bevy",
        
        // 音频
        "audio", "sound", "music", "3d_audio", "spatial_audio", "fmod", "wwise",
        "openal", "alut", "portaudio", "alsa", "pulseaudio", "directsound", "xaudio2",
        
        // 输入系统
        "input", "keyboard", "mouse", "joystick", "gamepad", "touch", "gesture",
        "vr", "ar", "oculus", "vive", "hololens", "leap_motion", "kinect", "leap",
        
        // 游戏逻辑
        "game_loop", "update", "render", "fixed_timestep", "delta_time", "fps",
        "state_machine", "behavior_tree", "ai", "pathfinding", "steering", "flocking"
    };
    
    // 图形渲染关键词（新增）
    project_indicators_["graphics_rendering"] = {
        // 渲染管线
        "rendering_pipeline", "graphics_pipeline", "vertex_stage", "fragment_stage",
        "geometry_stage", "tessellation_stage", "compute_stage", "rasterization",
        "clipping", "culling", "backface_culling", "frustum_culling", "occlusion_culling",
        
        // 着色器
        "shader", "vertex_shader", "fragment_shader", "geometry_shader", "compute_shader",
        "tessellation_shader", "glsl", "hlsl", "cg", "spirv", "shader_compilation",
        "shader_linking", "uniform", "attribute", "varying", "in", "out", "inout",
        
        // 材质系统
        "material", "texture", "diffuse", "specular", "normal", "bump", "displacement",
        "roughness", "metallic", "emissive", "albedo", "ao", "ambient_occlusion",
        "pbr", "physically_based_rendering", "brdf", "cook_torrance", "lambert",
        
        // 光照系统
        "lighting", "ambient", "directional", "point", "spot", "area", "shadow",
        "shadow_mapping", "shadow_volume", "cascaded_shadow_maps", "soft_shadows",
        "global_illumination", "radiosity", "photon_mapping", "path_tracing",
        
        // 后处理
        "post_processing", "bloom", "hdr", "tone_mapping", "gamma_correction",
        "anti_aliasing", "fxaa", "msaa", "ssaa", "taa", "temporal_anti_aliasing",
        "motion_blur", "depth_of_field", "ssao", "screen_space_ambient_occlusion"
    };
    
    // 音频处理关键词（新增）
    project_indicators_["audio_processing"] = {
        // 音频框架
        "audio", "sound", "music", "audio_processing", "audio_analysis", "audio_synthesis",
        "ffmpeg", "gstreamer", "portaudio", "alsa", "pulseaudio", "jack", "coreaudio",
        "directsound", "wasapi", "asio", "openal", "fmod", "wwise", "irrklang",
        
        // 音频格式
        "wav", "mp3", "aac", "flac", "ogg", "vorbis", "opus", "pcm", "adpcm",
        "midi", "mod", "s3m", "xm", "it", "tracker", "chiptune", "8bit", "16bit",
        
        // 音频处理
        "dsp", "digital_signal_processing", "fft", "dft", "fourier_transform",
        "filter", "low_pass", "high_pass", "band_pass", "notch", "eq", "equalizer",
        "compressor", "limiter", "gate", "reverb", "echo", "delay", "chorus", "flanger",
        
        // 3D音频
        "3d_audio", "spatial_audio", "binaural", "hrir", "head_related_impulse_response",
        "doppler", "attenuation", "occlusion", "obstruction", "reverb", "ambisonics",
        "surround_sound", "stereo", "mono", "quad", "5.1", "7.1", "atmos",
        
        // 音频分析
        "spectrum", "frequency", "amplitude", "phase", "envelope", "attack", "decay",
        "sustain", "release", "adsr", "pitch", "fundamental", "harmonic", "formant"
    };
    
    // 科学计算关键词（增强版）
    project_indicators_["scientific_computing"] = {
        // 数学库
        "eigen", "armadillo", "blas", "lapack", "atlas", "mkl", "openblas", "cublas",
        "gsl", "boost_math", "ceres", "g2o", "cholmod", "umfpack", "superlu",
        
        // 数值方法
        "numerical", "linear_algebra", "matrix", "vector", "tensor", "sparse_matrix",
        "eigenvalue", "eigenvector", "svd", "qr_decomposition", "lu_decomposition",
        "cholesky", "iterative_solver", "conjugate_gradient", "gmres", "bicgstab",
        
        // 科学计算
        "statistics", "probability", "random", "monte_carlo", "simulation", "optimization",
        "gradient_descent", "newton_method", "levenberg_marquardt", "genetic_algorithm",
        "particle_swarm", "simulated_annealing", "branch_and_bound",
        
        // 专业领域
        "finite_element", "finite_difference", "finite_volume", "mesh", "grid",
        "computational_fluid_dynamics", "cfd", "heat_transfer", "electromagnetics",
        "structural_analysis", "vibration", "acoustics", "optics", "quantum",
        
        // 并行计算
        "openmp", "mpi", "cuda", "opencl", "sycl", "hip", "rocm", "tbb", "cilk",
        "parallel", "distributed", "cluster", "grid_computing", "cloud_computing"
    };
    
    // 机器学习关键词（增强版）
    project_indicators_["machine_learning"] = {
        // 深度学习框架
        "tensorflow", "pytorch", "caffe", "caffe2", "mxnet", "chainer", "theano",
        "keras", "lasagne", "blocks", "fuel", "nolearn", "sklearn", "scikit_learn",
        
        // 神经网络
        "neural_network", "deep_learning", "cnn", "rnn", "lstm", "gru", "transformer",
        "attention", "self_attention", "multi_head_attention", "bert", "gpt",
        "resnet", "vgg", "alexnet", "inception", "densenet", "mobilenet", "efficientnet",
        
        // 机器学习算法
        "backpropagation", "gradient_descent", "adam", "sgd", "rmsprop", "adagrad",
        "dropout", "batch_normalization", "layer_normalization", "regularization",
        "l1_regularization", "l2_regularization", "elastic_net", "ridge", "lasso",
        
        // 计算机视觉
        "opencv", "computer_vision", "image_processing", "object_detection",
        "face_recognition", "ocr", "optical_character_recognition", "segmentation",
        "classification", "regression", "clustering", "dimensionality_reduction",
        "pca", "ica", "tsne", "umap", "manifold_learning",
        
        // 自然语言处理
        "nlp", "natural_language_processing", "tokenization", "stemming", "lemmatization",
        "word_embedding", "word2vec", "glove", "fasttext", "elmo", "ulmfit",
        "sentiment_analysis", "named_entity_recognition", "ner", "part_of_speech",
        "pos_tagging", "dependency_parsing", "semantic_parsing", "question_answering",
        
        // 数据处理
        "data_preprocessing", "feature_engineering", "feature_selection", "data_augmentation",
        "cross_validation", "k_fold", "stratified", "holdout", "bootstrap", "jackknife",
        "overfitting", "underfitting", "bias_variance", "model_selection", "hyperparameter",
        "grid_search", "random_search", "bayesian_optimization", "optuna", "hyperopt"
    };
    
    // 区块链关键词（新增）
    project_indicators_["blockchain"] = {
        // 区块链基础
        "blockchain", "distributed_ledger", "consensus", "proof_of_work", "pow",
        "proof_of_stake", "pos", "delegated_proof_of_stake", "dpos", "practical_byzantine",
        "pbft", "raft", "paxos", "tendermint", "hashgraph", "dag", "directed_acyclic",
        
        // 加密货币
        "bitcoin", "ethereum", "cryptocurrency", "digital_currency", "coin", "token",
        "altcoin", "fork", "hard_fork", "soft_fork", "segwit", "lightning_network",
        
        // 智能合约
        "smart_contract", "solidity", "vyper", "serpent", "lll", "evm", "ethereum_virtual",
        "web3", "dapp", "decentralized_application", "defi", "decentralized_finance",
        
        // 密码学
        "cryptography", "hash", "sha256", "sha3", "keccak", "ripemd", "blake", "scrypt",
        "pbkdf2", "bcrypt", "argon2", "merkle_tree", "merkle_root", "merkle_proof",
        "digital_signature", "ecdsa", "ed25519", "secp256k1", "curve25519",
        
        // 网络协议
        "p2p", "peer_to_peer", "gossip", "flooding", "kademlia", "dht", "distributed_hash",
        "bittorrent", "tor", "i2p", "freenet", "gnunet", "ipfs", "interplanetary_file",
        
        // 存储和数据库
        "ipfs", "swarm", "orbitdb", "gun", "blockstack", "arweave", "filecoin",
        "leveldb", "rocksdb", "lmdb", "sqlite", "postgresql", "mongodb", "redis"
    };
    
    // 数据库关键词（新增）
    project_indicators_["database"] = {
        // 关系型数据库
        "sql", "mysql", "postgresql", "sqlite", "oracle", "sql_server", "db2",
        "relational", "acid", "transaction", "isolation", "consistency", "durability",
        "normalization", "denormalization", "index", "b_tree", "hash_index", "bitmap",
        
        // NoSQL数据库
        "nosql", "mongodb", "couchdb", "couchbase", "document", "key_value", "redis",
        "memcached", "riak", "dynamodb", "cassandra", "hbase", "bigtable", "neo4j",
        "graph_database", "orientdb", "arangodb", "infinitegraph", "allegrograph",
        
        // 搜索引擎
        "elasticsearch", "solr", "lucene", "full_text_search", "inverted_index",
        "tf_idf", "bm25", "relevance", "ranking", "scoring", "faceted_search",
        
        // 时序数据库
        "influxdb", "timescaledb", "opentsdb", "kairosdb", "prometheus", "grafana",
        "time_series", "metrics", "monitoring", "alerting", "dashboard", "visualization",
        
        // 数据仓库
        "data_warehouse", "olap", "olap", "etl", "extract_transform_load", "elt",
        "star_schema", "snowflake_schema", "fact_table", "dimension_table", "cube",
        "hadoop", "hdfs", "mapreduce", "spark", "hive", "pig", "hbase", "kafka"
    };
    
    // 网络编程关键词（新增）
    project_indicators_["networking"] = {
        // 网络协议
        "tcp", "udp", "ip", "ipv4", "ipv6", "icmp", "arp", "rarp", "dhcp", "dns",
        "http", "https", "ftp", "smtp", "pop3", "imap", "ssh", "telnet", "snmp",
        "ldap", "kerberos", "ntp", "sntp", "rtp", "rtcp", "sip", "h323", "mgcp",
        
        // 网络编程
        "socket", "bind", "listen", "accept", "connect", "send", "recv", "close",
        "select", "poll", "epoll", "kqueue", "iocp", "completion_port", "overlapped",
        "async", "asynchronous", "non_blocking", "blocking", "synchronous",
        
        // 网络框架
        "boost_asio", "libevent", "libev", "libuv", "poco", "cpprest", "crow",
        "pistache", "drogon", "oatpp", "cpp_httplib", "beast", "cpprestsdk",
        
        // 负载均衡
        "load_balancer", "round_robin", "weighted_round_robin", "least_connections",
        "ip_hash", "consistent_hash", "sticky_session", "session_affinity",
        
        // 缓存和CDN
        "cdn", "content_delivery", "edge_server", "cache", "memcached", "redis",
        "varnish", "squid", "nginx", "apache", "traefik", "envoy", "istio",
        
        // 消息队列
        "message_queue", "rabbitmq", "kafka", "activemq", "zeromq", "nanomsg",
        "pub_sub", "publish_subscribe", "producer", "consumer", "broker", "topic"
    };
    
    // 包管理器关键词（新增）
    project_indicators_["package_manager"] = {
        // 包管理概念
        "package_manager", "package", "dependency", "dependency_management", "resolve",
        "install", "uninstall", "update", "upgrade", "downgrade", "version", "semver",
        "semantic_versioning", "lock_file", "lockfile", "manifest", "metadata",
        
        // 包管理器
        "npm", "yarn", "pnpm", "pip", "conda", "mamba", "apt", "yum", "dnf", "pacman",
        "brew", "portage", "pkg", "pkg_add", "pkg_install", "vcpkg", "conan", "hunter",
        "cget", "build2", "xmake", "meson", "cmake", "bazel", "buck", "pants",
        
        // 包格式
        "wheel", "egg", "tar", "gz", "zip", "deb", "rpm", "msi", "dmg", "pkg", "apk",
        "snap", "flatpak", "appimage", "nix", "guix", "spack", "easybuild",
        
        // 依赖解析
        "dependency_resolution", "conflict_resolution", "transitive_dependency",
        "direct_dependency", "indirect_dependency", "peer_dependency", "dev_dependency",
        "optional_dependency", "bundled_dependency", "system_dependency",
        
        // 版本控制
        "version_constraint", "version_range", "caret", "tilde", "exact_version",
        "latest", "stable", "beta", "alpha", "rc", "release_candidate", "pre_release",
        
        // 仓库管理
        "repository", "registry", "index", "mirror", "cache", "local_cache", "remote",
        "private_registry", "public_registry", "organization", "scope", "namespace",
        
        // 安全
        "security_audit", "vulnerability_scan", "license_check", "compliance",
        "signed_package", "checksum", "hash", "integrity", "authenticity", "trust",
        
        // 构建集成
        "build_system", "cmake", "make", "ninja", "msbuild", "gradle", "maven",
        "sbt", "cargo", "go_mod", "composer", "bundler", "gem", "pub", "cocoapods"
    };
    
    // 终端工具关键词（新增）
    project_indicators_["terminal_tool"] = {
        // 终端基础
        "terminal", "console", "shell", "command_line", "cli", "tui", "cui",
        "interactive", "non_interactive", "batch", "script", "automation",
        
        // 命令行界面
        "argparse", "getopt", "getopt_long", "boost_program_options", "cxxopts",
        "cli11", "docopt", "tclap", "gflags", "google_flags", "option_parser",
        "command_parser", "subcommand", "positional", "optional", "flag", "switch",
        
        // 终端控制
        "ansi", "escape_sequence", "color", "bold", "italic", "underline", "blink",
        "cursor", "clear", "scroll", "bell", "beep", "sound", "notification",
        
        // 输入输出
        "stdin", "stdout", "stderr", "pipe", "redirect", "tee", "cat", "grep",
        "sed", "awk", "sort", "uniq", "cut", "paste", "join", "comm", "diff",
        
        // 文件操作
        "file_operation", "copy", "move", "rename", "delete", "create", "list",
        "find", "locate", "which", "whereis", "type", "hash", "alias", "function",
        
        // 进程管理
        "process", "pid", "ppid", "fork", "exec", "wait", "signal", "kill",
        "background", "foreground", "job", "job_control", "nohup", "disown",
        
        // 系统信息
        "system_info", "uname", "hostname", "whoami", "id", "groups", "env",
        "printenv", "set", "export", "unset", "readonly", "declare", "typeset",
        
        // 网络工具
        "curl", "wget", "httpie", "postman", "insomnia", "ping", "traceroute",
        "nslookup", "dig", "host", "telnet", "nc", "netcat", "socat", "ssh",
        
        // 文本处理
        "text_processing", "regex", "pattern", "match", "replace", "substitute",
        "search", "filter", "transform", "format", "parse", "validate", "encode",
        "decode", "base64", "hex", "binary", "ascii", "utf8", "unicode",
        
        // 开发工具
        "git", "svn", "hg", "bzr", "fossil", "darcs", "monotone", "arch",
        "version_control", "scm", "source_control", "revision_control", "vcs"
    };
    
    // 系统工具关键词（新增）
    project_indicators_["system_tool"] = {
        // 系统监控
        "monitoring", "metrics", "telemetry", "observability", "logging", "tracing",
        "profiling", "benchmarking", "performance", "latency", "throughput", "qps",
        "cpu_usage", "memory_usage", "disk_usage", "network_usage", "io_usage",
        
        // 系统调用
        "syscall", "system_call", "kernel", "userspace", "kernelspace", "privilege",
        "root", "sudo", "su", "setuid", "setgid", "capability", "seccomp", "apparmor",
        
        // 文件系统
        "filesystem", "vfs", "virtual_filesystem", "mount", "umount", "fstab",
        "inode", "block", "sector", "cluster", "fragmentation", "defragmentation",
        "fsck", "checkdisk", "chkdsk", "badblocks", "smartctl", "hdparm",
        
        // 内存管理
        "memory_management", "malloc", "free", "calloc", "realloc", "mmap", "munmap",
        "virtual_memory", "physical_memory", "swap", "paging", "segmentation",
        "heap", "stack", "bss", "data", "text", "code", "rodata", "rwdata",
        
        // 进程间通信
        "ipc", "interprocess_communication", "pipe", "fifo", "named_pipe", "socket",
        "shared_memory", "shm", "semaphore", "mutex", "condition_variable", "barrier",
        "message_queue", "mq", "signal", "event", "wait", "notify", "broadcast",
        
        // 多线程
        "multithreading", "thread", "pthread", "std_thread", "boost_thread", "tbb",
        "openmp", "cilk", "cilkplus", "task", "future", "promise", "async", "await",
        "coroutine", "fiber", "green_thread", "user_thread", "kernel_thread",
        
        // 设备管理
        "device", "driver", "kernel_driver", "userspace_driver", "udev", "devfs",
        "procfs", "sysfs", "debugfs", "tracefs", "cgroup", "namespace", "container",
        "lxc", "docker", "podman", "runc", "crun", "containerd", "cri_o",
        
        // 安全
        "security", "selinux", "apparmor", "grsecurity", "paX", "stack_smashing",
        "buffer_overflow", "format_string", "use_after_free", "double_free",
        "memory_leak", "race_condition", "deadlock", "livelock", "starvation",
        
        // 网络
        "networking", "socket", "tcp", "udp", "ip", "routing", "firewall", "iptables",
        "netfilter", "ebpf", "xdp", "dpdk", "netmap", "pf_ring", "packet_capture",
        "tcpdump", "wireshark", "tshark", "tcpflow", "ngrep", "netstat", "ss"
    };
    
    // 开发工具关键词（新增）
    project_indicators_["development_tool"] = {
        // 构建工具
        "build_tool", "make", "cmake", "ninja", "bazel", "buck", "pants", "please",
        "gradle", "maven", "sbt", "ant", "ivy", "scons", "waf", "meson", "xmake",
        "premake", "qmake", "qbs", "tup", "redo", "just", "shake", "dune",
        
        // 编译器
        "compiler", "gcc", "g++", "clang", "clang++", "msvc", "icc", "icpc",
        "pgcc", "pgc++", "nvc", "nvc++", "armcc", "armclang", "keil", "iar",
        "cross_compiler", "cross_platform", "target", "host", "toolchain",
        
        // 调试器
        "debugger", "gdb", "lldb", "cdb", "windbg", "visual_studio", "eclipse",
        "netbeans", "codeblocks", "dev_c++", "qt_creator", "clion", "rider",
        "breakpoint", "watchpoint", "catchpoint", "tracepoint", "logpoint",
        
        // 静态分析
        "static_analysis", "clang_static_analyzer", "cppcheck", "cpplint",
        "pvs_studio", "coverity", "sonarqube", "splint", "flawfinder", "rats",
        "linter", "formatter", "clang_format", "astyle", "uncrustify", "indent",
        
        // 动态分析
        "dynamic_analysis", "valgrind", "memcheck", "helgrind", "drd", "massif",
        "callgrind", "cachegrind", "sanitizer", "address_sanitizer", "thread_sanitizer",
        "memory_sanitizer", "undefined_behavior_sanitizer", "leak_sanitizer",
        
        // 性能分析
        "profiler", "gprof", "perf", "oprofile", "intel_vtune", "amd_codexl",
        "nvidia_nsight", "arm_streamline", "valgrind_callgrind", "google_perftools",
        "tcmalloc", "jemalloc", "mimalloc", "hoard", "tbb_malloc", "lockless",
        
        // 测试框架
        "testing", "unit_test", "integration_test", "system_test", "regression_test",
        "gtest", "catch2", "boost_test", "doctest", "unity", "cppunit", "cxxtest",
        "mock", "stub", "fake", "spy", "test_double", "test_harness", "fixture",
        
        // 持续集成
        "ci_cd", "continuous_integration", "continuous_deployment", "continuous_delivery",
        "jenkins", "travis", "circleci", "github_actions", "gitlab_ci", "azure_devops",
        "bamboo", "teamcity", "buildkite", "drone", "concourse", "spinnaker",
        
        // 版本控制
        "version_control", "git", "svn", "hg", "bzr", "fossil", "darcs", "monotone",
        "branch", "merge", "rebase", "cherry_pick", "stash", "tag", "release",
        "pull_request", "merge_request", "code_review", "peer_review", "approval"
    };
    
    // 多媒体工具关键词（新增）
    project_indicators_["multimedia_tool"] = {
        // 音频处理
        "audio", "sound", "music", "audio_processing", "audio_analysis", "audio_synthesis",
        "ffmpeg", "gstreamer", "portaudio", "alsa", "pulseaudio", "jack", "coreaudio",
        "directsound", "wasapi", "asio", "openal", "fmod", "wwise", "irrklang",
        
        // 视频处理
        "video", "video_processing", "video_analysis", "video_synthesis", "video_encoding",
        "video_decoding", "video_transcoding", "video_streaming", "video_capture",
        "opencv", "gstreamer", "ffmpeg", "libav", "x264", "x265", "vp8", "vp9", "av1",
        
        // 图像处理
        "image", "image_processing", "image_analysis", "image_synthesis", "computer_vision",
        "opencv", "pillow", "imagemagick", "gimp", "photoshop", "gdi", "gdi+", "cairo",
        "skia", "freetype", "harfbuzz", "pango", "fontconfig", "libpng", "libjpeg",
        
        // 3D图形
        "3d", "three_dimensional", "opengl", "vulkan", "directx", "metal", "webgl",
        "gles", "glsl", "hlsl", "shader", "vertex", "fragment", "compute", "geometry",
        "tessellation", "raytracing", "pathtracing", "global_illumination", "pbr",
        
        // 游戏引擎
        "game_engine", "unity", "unreal", "godot", "cryengine", "lumberyard", "source",
        "id_tech", "frostbite", "cryengine", "lumberyard", "source", "id_tech", "frostbite",
        "ogre", "irrlicht", "panda3d", "horde3d", "bgfx", "magnum", "three_js",
        
        // 物理引擎
        "physics", "physics_engine", "bullet", "box2d", "chipmunk", "havok", "physx",
        "newton", "ode", "reactphysics3d", "jolt", "rapier", "bevy", "rapier3d",
        "collision_detection", "collision_response", "rigid_body", "soft_body", "fluid",
        
        // 动画
        "animation", "keyframe", "interpolation", "easing", "tweening", "morphing",
        "skeletal_animation", "bone", "rig", "ik", "fk", "blend_tree", "state_machine",
        "timeline", "sequencer", "curve", "bezier", "spline", "cubic", "linear",
        
        // 用户界面
        "ui", "user_interface", "gui", "widget", "button", "label", "textbox", "listbox",
        "combobox", "checkbox", "radiobutton", "slider", "progressbar", "menubar",
        "toolbar", "statusbar", "dialog", "window", "form", "layout", "container"
    };
    
    // 安全工具关键词（新增）
    project_indicators_["security_tool"] = {
        // 加密算法
        "cryptography", "crypto", "encryption", "decryption", "cipher", "ciphertext",
        "plaintext", "key", "public_key", "private_key", "symmetric", "asymmetric",
        "rsa", "aes", "des", "3des", "blowfish", "twofish", "serpent", "camellia",
        "chacha20", "salsa20", "poly1305", "gcm", "ccm", "ocb", "eax", "xts",
        
        // 哈希算法
        "hash", "hashing", "sha1", "sha256", "sha512", "sha3", "keccak", "blake2",
        "ripemd", "md5", "md4", "md2", "whirlpool", "tiger", "skein", "groestl",
        "crc", "checksum", "digest", "fingerprint", "thumbprint", "signature",
        
        // 数字签名
        "signature", "digital_signature", "ecdsa", "eddsa", "ed25519", "ed448",
        "secp256k1", "secp256r1", "secp384r1", "secp521r1", "curve25519", "curve448",
        "dsa", "rsa_pss", "rsa_pkcs1", "ecdsa_p256", "ecdsa_p384", "ecdsa_p521",
        
        // 网络安全
        "ssl", "tls", "https", "certificate", "x509", "pki", "ca", "certificate_authority",
        "csr", "certificate_signing_request", "crl", "certificate_revocation_list",
        "ocsp", "online_certificate_status_protocol", "pinning", "certificate_pinning",
        
        // 认证协议
        "oauth", "oauth2", "openid", "openid_connect", "saml", "jwt", "jws", "jwe",
        "jose", "json_web_token", "bearer_token", "access_token", "refresh_token",
        "id_token", "authorization_code", "implicit", "client_credentials", "password",
        
        // 安全框架
        "spring_security", "shiro", "pac4j", "keycloak", "auth0", "firebase_auth",
        "aws_cognito", "azure_ad", "okta", "ping_identity", "forgerock", "sailpoint",
        
        // 安全测试
        "penetration_testing", "vulnerability_assessment", "security_audit",
        "code_review", "static_analysis", "dynamic_analysis", "fuzzing", "fuzz_testing",
        "buffer_overflow", "stack_overflow", "heap_overflow", "format_string",
        "use_after_free", "double_free", "memory_leak", "race_condition",
        
        // 安全工具
        "nmap", "nessus", "openvas", "metasploit", "burp_suite", "owasp_zap",
        "sqlmap", "john_ripper", "hashcat", "hydra", "medusa", "nikto", "dirb",
        "gobuster", "dirbuster", "wfuzz", "ffuf", "subfinder", "amass", "nuclei"
    };
}

void ProjectTypeConfig::initialize_performance_indicators() {
    performance_indicators_ = {
        // 高性能计算
        "high_performance", "hpc", "performance", "optimization", "profiling",
        "benchmark", "throughput", "latency", "bandwidth", "scalability",
        
        // 并行计算
        "parallel", "concurrent", "threading", "multithreading", "async", "await",
        "openmp", "mpi", "cuda", "opencl", "sycl", "hip", "rocm", "tbb", "cilk",
        "thread", "mutex", "condition_variable", "atomic", "lock_free", "wait_free",
        
        // 内存优化
        "memory_pool", "object_pool", "arena", "bump_allocator", "stack_allocator",
        "cache_friendly", "locality", "prefetch", "preload", "zero_copy", "copy_on_write",
        
        // SIMD和向量化
        "simd", "vectorized", "sse", "sse2", "sse3", "sse4", "avx", "avx2", "avx512",
        "neon", "altivec", "intrinsics", "vectorization", "auto_vectorization",
        
        // GPU计算
        "gpu", "cuda", "opencl", "sycl", "hip", "rocm", "compute_shader", "shader",
        "vulkan", "directx", "metal", "opengl", "webgl", "gles", "compute",
        
        // 实时系统
        "real_time", "rtos", "hard_real_time", "soft_real_time", "deadline",
        "scheduling", "priority", "preemptive", "cooperative", "interrupt",
        
        // 网络性能
        "low_latency", "high_throughput", "zero_copy", "kernel_bypass", "dpdk",
        "netmap", "pf_ring", "packet_capture", "packet_processing", "fast_path"
    };
}

void ProjectTypeConfig::initialize_security_indicators() {
    security_indicators_ = {
        // 基础安全
        "security", "secure", "authentication", "authorization", "access_control",
        "permission", "privilege", "role", "user", "identity", "credential",
        
        // 加密算法
        "crypto", "cryptography", "encryption", "decryption", "cipher", "ciphertext",
        "plaintext", "key", "public_key", "private_key", "symmetric", "asymmetric",
        "rsa", "aes", "des", "3des", "blowfish", "twofish", "serpent", "camellia",
        "chacha20", "salsa20", "poly1305", "gcm", "ccm", "ocb", "eax",
        
        // 哈希算法
        "hash", "hashing", "sha1", "sha256", "sha512", "sha3", "keccak", "blake2",
        "ripemd", "md5", "md4", "md2", "whirlpool", "tiger", "skein", "groestl",
        
        // 数字签名
        "signature", "digital_signature", "ecdsa", "eddsa", "ed25519", "ed448",
        "secp256k1", "secp256r1", "secp384r1", "secp521r1", "curve25519", "curve448",
        "dsa", "rsa_pss", "rsa_pkcs1", "ecdsa_p256", "ecdsa_p384", "ecdsa_p521",
        
        // 网络安全
        "ssl", "tls", "https", "certificate", "x509", "pki", "ca", "certificate_authority",
        "csr", "certificate_signing_request", "crl", "certificate_revocation_list",
        "ocsp", "online_certificate_status_protocol", "pinning", "certificate_pinning",
        
        // 认证协议
        "oauth", "oauth2", "openid", "openid_connect", "saml", "jwt", "jws", "jwe",
        "jose", "json_web_token", "bearer_token", "access_token", "refresh_token",
        "id_token", "authorization_code", "implicit", "client_credentials", "password",
        
        // 安全框架
        "spring_security", "shiro", "pac4j", "keycloak", "auth0", "firebase_auth",
        "aws_cognito", "azure_ad", "okta", "ping_identity", "forgerock", "sailpoint",
        
        // 安全测试
        "penetration_testing", "vulnerability_assessment", "security_audit",
        "code_review", "static_analysis", "dynamic_analysis", "fuzzing", "fuzz_testing",
        "buffer_overflow", "stack_overflow", "heap_overflow", "format_string",
        "use_after_free", "double_free", "memory_leak", "race_condition"
    };
}

void ProjectTypeConfig::initialize_testing_indicators() {
    testing_indicators_ = {
        // 测试框架
        "gtest", "google_test", "catch2", "catch", "boost_test", "doctest", "unity",
        "cppunit", "cxxtest", "igloo", "bandit", "lest", "snitch", "utest", "minunit",
        
        // 测试类型
        "unit_test", "integration_test", "system_test", "acceptance_test", "regression_test",
        "smoke_test", "sanity_test", "exploratory_test", "ad_hoc_test", "monkey_test",
        "stress_test", "load_test", "performance_test", "volume_test", "scalability_test",
        "security_test", "usability_test", "accessibility_test", "compatibility_test",
        
        // 测试概念
        "test", "testing", "test_case", "test_suite", "test_fixture", "test_double",
        "mock", "stub", "fake", "spy", "dummy", "test_driver", "test_harness",
        "assertion", "assert", "expect", "verify", "check", "validate", "confirm",
        
        // 测试覆盖率
        "coverage", "code_coverage", "branch_coverage", "line_coverage", "function_coverage",
        "statement_coverage", "condition_coverage", "path_coverage", "mcdc_coverage",
        "gcov", "lcov", "bullseye", "coverity", "sonarqube", "codecov", "coveralls",
        
        // 持续集成
        "ci", "cd", "continuous_integration", "continuous_deployment", "continuous_delivery",
        "jenkins", "travis", "circleci", "github_actions", "gitlab_ci", "azure_devops",
        "bamboo", "teamcity", "buildkite", "drone", "concourse", "spinnaker",
        
        // 测试驱动开发
        "tdd", "test_driven_development", "bdd", "behavior_driven_development",
        "atdd", "acceptance_test_driven_development", "red_green_refactor",
        "given_when_then", "arrange_act_assert", "setup_exercise_verify_teardown",
        
        // 测试自动化
        "test_automation", "automated_testing", "selenium", "webdriver", "playwright",
        "cypress", "puppeteer", "nightwatch", "protractor", "karma", "jasmine",
        "mocha", "jest", "vitest", "ava", "tape", "tap", "node_tap"
    };
}

void ProjectTypeConfig::initialize_ml_features() {
    ml_features_ = {
        // 神经网络架构
        "neural_network", "deep_learning", "cnn", "convolutional_neural_network",
        "rnn", "recurrent_neural_network", "lstm", "long_short_term_memory",
        "gru", "gated_recurrent_unit", "transformer", "attention", "self_attention",
        "multi_head_attention", "bert", "gpt", "t5", "roberta", "albert", "xlnet",
        
        // 深度学习模型
        "resnet", "residual_network", "vgg", "alexnet", "inception", "densenet",
        "mobilenet", "efficientnet", "yolo", "rcnn", "faster_rcnn", "mask_rcnn",
        "ssd", "single_shot_detector", "retinanet", "fpn", "feature_pyramid_network",
        
        // 优化算法
        "backpropagation", "gradient_descent", "stochastic_gradient_descent", "sgd",
        "adam", "adaptive_moment_estimation", "rmsprop", "adagrad", "adadelta",
        "adamax", "nadam", "amsgrad", "radam", "lookahead", "ranger", "lamb",
        
        // 正则化技术
        "dropout", "batch_normalization", "layer_normalization", "group_normalization",
        "instance_normalization", "weight_normalization", "spectral_normalization",
        "l1_regularization", "l2_regularization", "elastic_net", "ridge", "lasso",
        "early_stopping", "data_augmentation", "mixup", "cutmix", "cutout", "random_erasing",
        
        // 机器学习算法
        "supervised_learning", "unsupervised_learning", "semi_supervised_learning",
        "reinforcement_learning", "classification", "regression", "clustering",
        "dimensionality_reduction", "pca", "principal_component_analysis", "ica",
        "lda", "linear_discriminant_analysis", "tsne", "umap", "manifold_learning",
        
        // 计算机视觉
        "computer_vision", "image_processing", "object_detection", "object_recognition",
        "face_recognition", "face_detection", "ocr", "optical_character_recognition",
        "image_segmentation", "semantic_segmentation", "instance_segmentation",
        "panoptic_segmentation", "keypoint_detection", "pose_estimation", "tracking",
        
        // 自然语言处理
        "nlp", "natural_language_processing", "tokenization", "stemming", "lemmatization",
        "word_embedding", "word2vec", "glove", "fasttext", "elmo", "ulmfit", "flair",
        "sentiment_analysis", "named_entity_recognition", "ner", "part_of_speech",
        "pos_tagging", "dependency_parsing", "semantic_parsing", "question_answering",
        "machine_translation", "text_summarization", "text_generation", "dialogue_system",
        
        // 数据处理
        "data_preprocessing", "feature_engineering", "feature_selection", "feature_extraction",
        "cross_validation", "k_fold", "stratified", "holdout", "bootstrap", "jackknife",
        "overfitting", "underfitting", "bias_variance", "model_selection", "hyperparameter",
        "grid_search", "random_search", "bayesian_optimization", "optuna", "hyperopt"
    };
}

void ProjectTypeConfig::initialize_code_quality_indicators() {
    code_quality_indicators_ = {
        // 现代C++特性
        "const", "constexpr", "noexcept", "override", "final", "explicit", "virtual",
        "pure_virtual", "abstract", "interface", "traits", "concepts", "requires",
        
        // 智能指针
        "smart_pointer", "unique_ptr", "shared_ptr", "weak_ptr", "auto_ptr", "scoped_ptr",
        "intrusive_ptr", "raw_pointer", "dangling_pointer", "memory_leak", "double_delete",
        
        // RAII和资源管理
        "raii", "resource_acquisition_is_initialization", "scope_guard", "finally",
        "lock_guard", "unique_lock", "shared_lock", "scoped_lock", "mutex", "condition_variable",
        
        // 移动语义
        "move_semantics", "rvalue_reference", "std_move", "std_forward", "perfect_forwarding",
        "universal_reference", "forwarding_reference", "decltype", "auto", "type_deduction",
        
        // 模板元编程
        "template_metaprogramming", "templates", "typename", "template", "specialization",
        "partial_specialization", "explicit_specialization", "variadic_templates", "parameter_pack",
        "fold_expressions", "if_constexpr", "constexpr_if", "sfinae", "enable_if",
        
        // 现代C++标准
        "c++11", "c++14", "c++17", "c++20", "c++23", "ranges", "coroutines", "modules",
        "concepts", "requires", "constraints", "ranges", "std_ranges", "views", "algorithms",
        
        // 异常安全
        "exception_safety", "noexcept", "strong_exception_safety", "basic_exception_safety",
        "no_throw_guarantee", "exception_specification", "std_terminate", "std_unexpected",
        
        // 性能优化
        "performance", "optimization", "profiling", "benchmark", "cache_friendly", "locality",
        "branch_prediction", "cpu_cache", "memory_alignment", "data_structures", "algorithms",
        
        // 代码风格
        "naming_convention", "coding_style", "indentation", "braces", "spacing", "comments",
        "documentation", "doxygen", "javadoc", "readme", "changelog", "version_control"
    };
}

void ProjectTypeConfig::initialize_architecture_patterns() {
    architecture_patterns_ = {
        // 设计模式
        "singleton", "factory", "abstract_factory", "builder", "prototype", "object_pool",
        "observer", "publisher_subscriber", "strategy", "command", "state", "visitor",
        "template_method", "chain_of_responsibility", "mediator", "memento", "interpreter",
        "iterator", "composite", "decorator", "facade", "proxy", "bridge", "flyweight",
        
        // 架构模式
        "mvc", "model_view_controller", "mvp", "model_view_presenter", "mvvm", "model_view_viewmodel",
        "microservice", "microservices", "soa", "service_oriented_architecture", "monolith",
        "event_driven", "event_sourcing", "cqrs", "command_query_responsibility_segregation",
        "reactive", "reactive_programming", "actor_model", "akka", "erlang", "elixir",
        
        // 分布式系统
        "distributed_system", "distributed_computing", "cluster", "grid_computing",
        "cloud_computing", "edge_computing", "fog_computing", "serverless", "lambda",
        "container", "docker", "kubernetes", "orchestration", "service_mesh", "istio",
        
        // 数据架构
        "database", "relational", "nosql", "document", "key_value", "graph", "time_series",
        "data_warehouse", "data_lake", "data_mart", "etl", "elt", "olap", "oltp",
        "data_pipeline", "data_streaming", "kafka", "rabbitmq", "message_queue",
        
        // 网络架构
        "client_server", "peer_to_peer", "p2p", "rest", "graphql", "grpc", "soap",
        "api_gateway", "load_balancer", "reverse_proxy", "cdn", "cache", "session",
        "stateless", "stateful", "scalability", "availability", "reliability",
        
        // 安全架构
        "security", "authentication", "authorization", "oauth", "jwt", "saml", "ldap",
        "rbac", "role_based_access_control", "abac", "attribute_based_access_control",
        "zero_trust", "defense_in_depth", "security_by_design", "privacy_by_design"
    };
}

// 获取项目类型指示器
const std::vector<std::string>& ProjectTypeConfig::get_project_indicators(const std::string& project_type) const {
    auto it = project_indicators_.find(project_type);
    if (it != project_indicators_.end()) {
        return it->second;
    }
    static const std::vector<std::string> empty;
    return empty;
}

// 获取所有项目类型
std::vector<std::string> ProjectTypeConfig::get_all_project_types() const {
    std::vector<std::string> types;
    for (const auto& pair : project_indicators_) {
        types.push_back(pair.first);
    }
    return types;
}

// 获取性能指示器
const std::vector<std::string>& ProjectTypeConfig::get_performance_indicators() const {
    return performance_indicators_;
}

// 获取安全指示器
const std::vector<std::string>& ProjectTypeConfig::get_security_indicators() const {
    return security_indicators_;
}

// 获取测试指示器
const std::vector<std::string>& ProjectTypeConfig::get_testing_indicators() const {
    return testing_indicators_;
}

// 获取机器学习特征
const std::vector<std::string>& ProjectTypeConfig::get_ml_features() const {
    return ml_features_;
}

// 获取代码质量指示器
const std::vector<std::string>& ProjectTypeConfig::get_code_quality_indicators() const {
    return code_quality_indicators_;
}

// 获取架构模式
const std::vector<std::string>& ProjectTypeConfig::get_architecture_patterns() const {
    return architecture_patterns_;
}

} // namespace Analysis
} // namespace Paker
