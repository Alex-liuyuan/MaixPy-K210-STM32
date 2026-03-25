"""
tests/test_image.py
Image像素操作测试（numpy实现验证）
"""

import pytest
import numpy as np
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), ".."))


@pytest.fixture
def rgb_image():
    """创建一个320x240的黑色RGB888图像"""
    from maix.camera import Image
    data = np.zeros((240, 320, 3), dtype=np.uint8)
    return Image(data, 320, 240, "RGB888")


def test_image_create(rgb_image):
    assert rgb_image.width  == 320
    assert rgb_image.height == 240
    assert rgb_image.format == "RGB888"
    assert rgb_image.data.shape == (240, 320, 3)


def test_draw_rectangle_changes_pixels(rgb_image):
    img = rgb_image
    img.draw_rectangle(10, 10, 50, 30, color=(255, 0, 0), thickness=2)
    # 上边框应为红色
    assert img.data[10, 10, 0] == 255
    assert img.data[10, 10, 1] == 0
    assert img.data[10, 10, 2] == 0
    # 内部应仍为黑色
    assert img.data[25, 35, 0] == 0


def test_draw_rectangle_returns_self(rgb_image):
    result = rgb_image.draw_rectangle(0, 0, 10, 10)
    assert result is rgb_image


def test_draw_line_changes_pixels(rgb_image):
    img = rgb_image
    img.draw_line(0, 0, 10, 0, color=(0, 255, 0), thickness=1)
    # 水平线上的像素应为绿色
    assert img.data[0, 5, 1] == 255


def test_draw_string_changes_pixels(rgb_image):
    img = rgb_image
    # 绘制前全黑
    assert img.data[0, 0, 0] == 0
    img.draw_string(0, 0, "A", color=(255, 255, 255), scale=1)
    # 'A'字符应在某些像素上有白色
    total_white = np.sum(img.data[:10, :10, 0] == 255)
    assert total_white > 0


def test_resize_shape():
    from maix.camera import Image
    data = np.random.randint(0, 255, (240, 320, 3), dtype=np.uint8)
    img = Image(data, 320, 240, "RGB888")
    resized = img.resize(160, 120)
    assert resized.width  == 160
    assert resized.height == 120
    assert resized.data.shape == (120, 160, 3)


def test_resize_nearest_neighbor():
    """验证最近邻插值：2x2图像放大到4x4"""
    from maix.camera import Image
    data = np.array([[[255, 0, 0], [0, 255, 0]],
                     [[0, 0, 255], [255, 255, 0]]], dtype=np.uint8)
    img = Image(data, 2, 2, "RGB888")
    big = img.resize(4, 4)
    assert big.data.shape == (4, 4, 3)
    # 左上角应仍为红色
    assert big.data[0, 0, 0] == 255
    assert big.data[0, 0, 1] == 0


def test_crop_shape():
    from maix.camera import Image
    data = np.random.randint(0, 255, (240, 320, 3), dtype=np.uint8)
    img = Image(data, 320, 240, "RGB888")
    cropped = img.crop(10, 20, 100, 80)
    assert cropped.width  == 100
    assert cropped.height == 80
    assert cropped.data.shape == (80, 100, 3)


def test_crop_pixel_values():
    """验证裁剪后像素值正确"""
    from maix.camera import Image
    data = np.zeros((100, 100, 3), dtype=np.uint8)
    data[10:20, 10:20] = [255, 128, 64]
    img = Image(data, 100, 100, "RGB888")
    cropped = img.crop(10, 10, 10, 10)
    assert np.all(cropped.data[:, :, 0] == 255)
    assert np.all(cropped.data[:, :, 1] == 128)


def test_copy_independence():
    from maix.camera import Image
    data = np.zeros((50, 50, 3), dtype=np.uint8)
    img = Image(data, 50, 50, "RGB888")
    copy = img.copy()
    copy.data[0, 0] = [255, 255, 255]
    assert img.data[0, 0, 0] == 0  # 原图不受影响


def test_to_bytes():
    from maix.camera import Image
    data = np.ones((10, 10, 3), dtype=np.uint8) * 42
    img = Image(data, 10, 10, "RGB888")
    b = img.to_bytes()
    assert isinstance(b, bytes)
    assert len(b) == 10 * 10 * 3
    assert b[0] == 42


def test_camera_read_returns_image():
    """测试Camera.read()返回Image对象"""
    from maix.camera import Camera
    cam = Camera(160, 120, "RGB888")
    img = cam.read()
    assert img is not None
    assert img.width  == 160
    assert img.height == 120
    cam.close()
