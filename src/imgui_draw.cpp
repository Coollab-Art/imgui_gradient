#include <imgui.h>
#include <vector>
#include "ColorRGBA.hpp"
#include "Gradient.hpp"
#include "Interpolation.hpp"
#include "Settings.hpp"
#include "color_conversions.hpp"
#include "internal.hpp"

namespace ImGG {

static void draw_uniform_square(
    ImDrawList&  draw_list,
    const ImVec2 top_left_corner,
    const ImVec2 bottom_right_corner,
    const ImU32& color
)
{
    static constexpr auto rounding{1.f};
    draw_list.AddRectFilled(
        top_left_corner,
        bottom_right_corner,
        color,
        rounding
    );
}

static void draw_naive_gradient_between_two_colors(
    ImDrawList&  draw_list,
    ImVec2 const top_left_corner,
    ImVec2 const bottom_right_corner,
    ImU32 const& color_left, ImU32 const& color_right
)
{
    draw_list.AddRectFilledMultiColor(
        top_left_corner,
        bottom_right_corner,
        color_left, color_right, color_right, color_left
    );
}

// We draw multiple "naive" gradients to reduce the visual artifacts.
// The problem is that ImGui does its color interpolation in sRGB space which doesn't look the best.
// We use Oklab color space and Premultiplied Alpha for better-looking results.
static void draw_gradient_between_two_colors(
    ImDrawList&      draw_list,
    ImVec2 const     top_left_corner,
    ImVec2 const     bottom_right_corner,
    ColorRGBA const& left_color,
    ColorRGBA const& right_color,
    int              subdivisions
)
{
    if (subdivisions == 0)
        draw_naive_gradient_between_two_colors(draw_list, top_left_corner, bottom_right_corner, ImGui::ColorConvertFloat4ToU32(left_color), ImGui::ColorConvertFloat4ToU32(right_color));

    auto const to_oklab = [](ImVec4 const& col) {
        return internal::Oklab_Premultiplied_from_sRGB_Straight(col);
    };
    auto const to_sRGB = [](ImVec4 const& col) {
        return ImGui::ColorConvertFloat4ToU32(internal::sRGB_Straight_from_Oklab_Premultiplied(col));
    };

    auto const left_oklab  = to_oklab(left_color);
    auto const right_oklab = to_oklab(right_color);

    // Create all the X coords and all the colors for the sub-gradients
    auto xs     = std::vector<float>{};
    auto colors = std::vector<ImU32>{};
    xs.push_back(top_left_corner.x);
    colors.push_back(ImGui::ColorConvertFloat4ToU32(left_color));
    for (int i = 0; i < subdivisions; ++i)
    {
        float const t = static_cast<float>(i + 1) / static_cast<float>(subdivisions + 1);
        xs.push_back(top_left_corner.x * (1.f - t) + bottom_right_corner.x * t);
        colors.push_back(to_sRGB(
            left_oklab * ImVec4{1.f - t, 1.f - t, 1.f - t, 1.f - t}
            + right_oklab * ImVec4{t, t, t, t}
        ));
    }
    xs.push_back(bottom_right_corner.x);
    colors.push_back(ImGui::ColorConvertFloat4ToU32(right_color));

    for (size_t i = 0; i < colors.size() - 1; ++i)
    {
        draw_naive_gradient_between_two_colors(
            draw_list,
            ImVec2{xs[i], top_left_corner.y},
            ImVec2{xs[i + 1], bottom_right_corner.y},
            colors[i],
            colors[i + 1]
        );
    }
}

void draw_gradient(
    ImDrawList&     draw_list,
    const Gradient& gradient,
    const ImVec2    gradient_position,
    const ImVec2    size,
    Settings const& settings
)
{
    assert(!gradient.is_empty());
    float current_starting_x = gradient_position.x;
    for (auto mark_iterator = gradient.get_marks().begin(); mark_iterator != gradient.get_marks().end(); ++mark_iterator) // We need to use iterators because we need to access the previous element from time to time
    {
        const Mark& mark = *mark_iterator;

        const auto color_right = mark.color;

        const auto from{current_starting_x};
        const auto to{gradient_position.x + mark.position.get() * size.x};
        if (mark.position.get() != 0.f)
        {
            if (gradient.interpolation_mode() == Interpolation::Linear)
            {
                const auto color_left = (mark_iterator != gradient.get_marks().begin())
                                            ? std::prev(mark_iterator)->color
                                            : color_right;
                draw_gradient_between_two_colors(
                    draw_list,
                    ImVec2{from - 0.55f, gradient_position.y},        // We extend the rectangle by 0.55 on each side, otherwise there can be small gaps that appear between rectangles
                    ImVec2{to + 0.55f, gradient_position.y + size.y}, // (for some gradient width and some mark positions) (see https://github.com/Coollab-Art/imgui_gradient/issues/5)
                    color_left, color_right,
                    settings.gradient_subdivisions
                );
            }
            else if (gradient.interpolation_mode() == Interpolation::Constant)
            {
                draw_uniform_square(
                    draw_list,
                    ImVec2{from - 0.55f, gradient_position.y},        // We extend the rectangle by 0.55 on each side, otherwise there can be small gaps that appear between rectangles
                    ImVec2{to + 0.55f, gradient_position.y + size.y}, // (for some gradient width and some mark positions) (see https://github.com/Coollab-Art/imgui_gradient/issues/5)
                    ImGui::ColorConvertFloat4ToU32(color_right)
                );
            }
            else
            {
                assert(false && "Unknown Interpolation enum value.");
            }
        }
        current_starting_x = to;
    }
    // If the last element is not at the end position, extend its color to the end position
    if (gradient.get_marks().back().position.get() != 1.f)
    {
        draw_uniform_square(
            draw_list,
            ImVec2{current_starting_x, gradient_position.y},
            ImVec2{gradient_position.x + size.x, gradient_position.y + size.y},
            ImGui::ColorConvertFloat4ToU32(gradient.get_marks().back().color)
        );
    }
}

static auto mark_invisible_button(
    const ImVec2 position_to_draw_mark,
    const float  mark_square_size,
    const float  gradient_height
) -> bool
{
    ImGui::SetCursorScreenPos(position_to_draw_mark - ImVec2{mark_square_size * 1.5f, gradient_height});
    const auto button_size = ImVec2{
        mark_square_size * 3.f,
        gradient_height + mark_square_size * 2.f
    };
    ImGui::InvisibleButton("mark", button_size, ImGuiButtonFlags_MouseButtonMiddle | ImGuiButtonFlags_MouseButtonLeft);
    return ImGui::IsItemHovered();
}

static void draw_mark(
    ImDrawList&  draw_list,
    const ImVec2 position_to_draw_mark,
    const float  mark_square_size,
    ImU32        mark_color
)
{
    const auto mark_top_triangle    = ImVec2{0.f, -mark_square_size};
    const auto mark_bottom_triangle = ImVec2{mark_square_size, 0.f};
    draw_list.AddTriangleFilled(
        position_to_draw_mark + mark_top_triangle,
        position_to_draw_mark - mark_bottom_triangle,
        position_to_draw_mark + mark_bottom_triangle,
        mark_color
    );

    const auto mark_top_left_corner =
        ImVec2{-mark_square_size - 1.f, 0.f};
    const auto mark_bottom_right_corner =
        ImVec2{
            mark_square_size + 1.f,
            2.f * mark_square_size
        };
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner,
        position_to_draw_mark + mark_bottom_right_corner,
        mark_color
    );

    static constexpr auto offset_between_mark_square_and_mark_square_inside = ImVec2{1.f, 1.f};
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner + offset_between_mark_square_and_mark_square_inside,
        position_to_draw_mark + mark_bottom_right_corner - offset_between_mark_square_and_mark_square_inside,
        mark_color
    );
}

void draw_marks(
    ImDrawList&     draw_list,
    const ImVec2    position_to_draw_mark,
    const ImU32     mark_color,
    const float     gradient_height,
    const bool      mark_is_selected,
    Settings const& settings
)
{
    float const mark_square_size = 0.3f * ImGui::GetFontSize();

    bool const is_hovered = mark_invisible_button(position_to_draw_mark, mark_square_size, gradient_height);

    draw_mark(
        draw_list,
        position_to_draw_mark,
        mark_square_size,
        is_hovered
            ? (mark_is_selected
                   ? settings.mark_selected_hovered_color
                   : settings.mark_hovered_color)
            : (mark_is_selected
                   ? settings.mark_selected_color
                   : settings.mark_color)
    );

    auto const square_size{0.15f * ImGui::GetFontSize()};
    auto const mark_top_left_corner     = ImVec2{-square_size, square_size};
    auto const mark_bottom_right_corner = ImVec2{square_size, square_size * 3};
    draw_uniform_square(
        draw_list,
        position_to_draw_mark + mark_top_left_corner,
        position_to_draw_mark + mark_bottom_right_corner,
        mark_color
    );
}

} // namespace ImGG