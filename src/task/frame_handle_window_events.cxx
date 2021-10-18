#include <vulkan/vulkan.h>
#include "frame_handle_window_events.hxx"

TASK_DECL {
  ZoneScoped;

  if (session_state->ignore_glfw_events) {
    return;
  }

  auto user_data = new engine::misc::GlfwUserData {
    .state = session_state.ptr,
  };
  glfwSetWindowUserPointer(glfw->window, user_data);
  glfwPollEvents();
  glfwSetWindowUserPointer(glfw->window, nullptr);
  delete user_data;

  auto cursor_delta = glm::vec2(0.0);
  auto is_focused = glfwGetWindowAttrib(glfw->window, GLFW_FOCUSED);
  { // update mouse position
    double x, y;
    int w, h;
    glfwGetCursorPos(glfw->window, &x, &y);
    glfwGetWindowSize(glfw->window, &w, &h);
    if (w > 0 && h > 0) {
      auto new_position = 2.0f * glm::vec2(x / w, y / h) - 1.0f;
      if (is_focused
        && !isnan(glfw->last_known_mouse_cursor_position.x)
        && !isnan(glfw->last_known_mouse_cursor_position.y)
      ) {
        cursor_delta = new_position - glfw->last_known_mouse_cursor_position;
      }
      glfw->last_known_mouse_cursor_position = new_position;
    } else {
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    }
  }
  { ZoneScopedN("fullscreen");
    auto monitor = glfwGetWindowMonitor(glfw->window);
    if (monitor == nullptr && session_state->is_fullscreen) {
      // save last position
      glfwGetWindowPos(glfw->window, &glfw->last_window_position.x, &glfw->last_window_position.y);
      glfwGetWindowSize(glfw->window, &glfw->last_window_size.x, &glfw->last_window_size.y);

      monitor = glfwGetPrimaryMonitor();
      auto mode = glfwGetVideoMode(monitor);

      glfwSetWindowMonitor(
        glfw->window, monitor,
        0,
        0,
        mode->width,
        mode->height,
        mode->refreshRate
      );
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    } else if (monitor != nullptr && !session_state->is_fullscreen) {
      glfwSetWindowMonitor(
        glfw->window, nullptr,
        glfw->last_window_position.x, 
        glfw->last_window_position.y, 
        glfw->last_window_size.x,
        glfw->last_window_size.y,
        GLFW_DONT_CARE
      );
      glfw->last_known_mouse_cursor_position = glm::vec2(NAN, NAN);
    }
  }
  { ZoneScopedN("cursor_mode");
    int desired_cursor_mode = (
      !is_focused || session_state->show_imgui
        ? GLFW_CURSOR_NORMAL
        : GLFW_CURSOR_DISABLED
    );
    int actual_cursor_mode = glfwGetInputMode(glfw->window, GLFW_CURSOR);
    if (desired_cursor_mode != actual_cursor_mode) {
      glfwSetInputMode(glfw->window, GLFW_CURSOR, desired_cursor_mode);
    }
  }
  { ZoneScopedN("update_data");
    auto it = &update->debug_camera_input;
    *it = {};
    if (!session_state->show_imgui) {
      it->cursor_position_delta = cursor_delta;

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_W) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_UP) == GLFW_PRESS)
      ) {
        it->y_pos = true;
      }

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_S) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_DOWN) == GLFW_PRESS)
      ) {
        it->y_neg = true;
      }

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_D) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_RIGHT) == GLFW_PRESS)
      ) {
        it->x_pos = true;
      }

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_A) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_LEFT) == GLFW_PRESS)
      ) {
        it->x_neg = true;
      }

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_E) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
      ) {
        it->z_pos = true;
      }

      if (0
        || (glfwGetKey(glfw->window, GLFW_KEY_Q) == GLFW_PRESS)
        || (glfwGetKey(glfw->window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
      ) {
        it->z_neg = true;
      }
    }
  }
}
