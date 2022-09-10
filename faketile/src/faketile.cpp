#include <wayfire/object.hpp>
#include <wayfire/plugin.hpp>
#include <wayfire/signal-definitions.hpp>
#include <wayfire/util/log.hpp>
#include <wayfire/workspace-manager.hpp>

class faketile_t : public wf::plugin_interface_t
{
    wf::signal_connection_t created_cb = [=] (wf::signal_data_t *data)
    {
        auto ev   = (wf::view_mapped_signal*)(data);
        auto view = get_signaled_view(data);
		nothing("mapped: ", view->to_string());

        if ((view->role != wf::VIEW_ROLE_TOPLEVEL) || view->parent ||
            view->fullscreen || view->tiled_edges || ev->is_positioned)
        {
            return;
        }

		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    this->output->workspace->get_current_workspace(), interesting_layers);
		retileAdded(views,view);
		ev->is_positioned = true;

        /* auto workarea = output->workspace->get_workarea(); */

    };
    wf::signal_connection_t removed_cb = [=] (wf::signal_data_t *data) {
        auto view = get_signaled_view(data);
		nothing("unmapped: ", view->to_string());
		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    this->output->workspace->get_current_workspace(), interesting_layers);
		retileRemoved(views, view);


	};

    wf::signal_connection_t minimized_cb = [=] (wf::signal_data_t *data) {
        auto ev   = (wf::view_minimized_signal*)(data);
        auto view = get_signaled_view(data);
		nothing("minimized: ", view->to_string());
		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    this->output->workspace->get_current_workspace(), interesting_layers);
		if(ev->state) //minimized
			retileRemoved(views, view);
		else //restored
			retileAdded(views, view);
	};

 
	template <typename T, typename... Types>
	inline void nothing(T var1, Types... var2)
	{
		/* LOGI(var1,var2...); */
	}

	void retileRemoved(std::vector<wayfire_view> views, wayfire_view view) {
		auto viewg = view->get_wm_geometry();
		nothing("retile removed: ", view->to_string());
		nothing("it was:",view->to_string(), view->get_title(), view->get_app_id(), viewg.x,"y", viewg.y,"w", viewg.width,"h", viewg.height);
		std::vector<wayfire_view> leftViews, rightViews, upViews, downViews;
		int leftHeight = 0, rightHeight = 0, upWidth = 0, downWidth = 0;
		for(auto v: views) {
			if(v == view) continue;

			auto vg = v->get_wm_geometry();
			nothing("curretn: ",v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			if(isEqualish(vg.x + vg.width, viewg.x) && isInsideInclusive(vg.y, vg.height, viewg.y, viewg.height)) { 
				leftViews.push_back(v); 
				leftHeight += vg.height;
				nothing("left:",v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			}
			else if(isEqualish(vg.x, viewg.x + viewg.width) && isInsideInclusive(vg.y, vg.height, viewg.y, viewg.height)) {
				rightViews.push_back(v);
				rightHeight += vg.height;
				nothing("right:",v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			}
			else if(isEqualish(vg.y + vg.height, viewg.y) && isInsideInclusive(vg.x, vg.width, viewg.x, viewg.width)) {
				upViews.push_back(v); 
				upWidth += vg.width;
				nothing("up:",v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			}
			else if(isEqualish(vg.y, viewg.y + viewg.height) && isInsideInclusive(vg.x, vg.width, viewg.x, viewg.width)) {
				downViews.push_back(v);
				downWidth += vg.width;
				nothing("down:",v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			}
		}
		if(isEqualish(leftHeight, viewg.height)) {
			for(auto v:leftViews) {
				auto vg = v->get_wm_geometry();
				v->resize(vg.width + viewg.width, vg.height);
			}
		}
		else if(isEqualish(rightHeight, viewg.height)) {
			for(auto v:rightViews) {
				auto vg = v->get_wm_geometry();
				v->resize(vg.width + viewg.width, vg.height);
				v->move(viewg.x, vg.y);
			}
		}
		else if(isEqualish(upWidth, viewg.width)) {
			for(auto v:upViews) {
				auto vg = v->get_wm_geometry();
				v->resize(vg.width, vg.height + viewg.height);
			}
		}
		else if(isEqualish(downWidth, viewg.width)) {
			for(auto v:downViews) {
				auto vg = v->get_wm_geometry();
				v->resize(vg.width, vg.height + viewg.height);
				v->move(vg.x, viewg.y);
			}
		}
	}
	inline bool isEqualish(int a, int b, int fuzz=10) {
		return a < b+fuzz && a > b-fuzz;
	}
	bool isInsideInclusive(int line1Start,int line1Length, int line2Start, int line2Length, int fuzz=10) {
		return line1Start >= line2Start-fuzz && line1Start+line1Length <= line2Length + line2Start + fuzz;
	}
	void retileAdded(std::vector<wayfire_view> views, wayfire_view view) {
		if(views.size() > 3) {
			retileAddedAfter3(views, view);
		}
		else {
			retileAddedUpto3(views);
		}
	}

	void retileAddedAfter3(std::vector<wayfire_view> views, wayfire_view view) {
		nothing("retile addedafter3: ", view->to_string());
		int size = views.size();
		unsigned int max_area=0;
		wayfire_view max_view = nullptr;
		for(int i=0; i<size; i++) {
			auto v = views[i];
			auto vg = v->get_wm_geometry();
			nothing(v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			if(v == view) continue;
			if(vg.x == 0 && vg.y == 0) continue;
			unsigned int area = vg.width * vg.height;
			if(area >= max_area) {
				max_area = area;
				max_view = v;
			}
		}
		if(max_view == nullptr || max_area == 0) return;
		auto max_view_g = max_view->get_wm_geometry();
		if(max_view_g.width > max_view_g.height) {
			/* view->set_geometry({max_view_g.width/2 + max_view_g.x, max_view_g.y, max_view_g.width/2, max_view_g.height}); */
			view->resize(max_view_g.width/2, max_view_g.height);
			view->move(max_view_g.width/2 + max_view_g.x, max_view_g.y);
			max_view->resize(max_view_g.width/2, max_view_g.height);
		}
		else {
			/* view->set_geometry({max_view_g.x, max_view_g.y + max_view_g.height/2, max_view_g.width, max_view_g.height/2}); */
			view->resize(max_view_g.width, max_view_g.height/2);
			view->move(max_view_g.x, max_view_g.y + max_view_g.height/2);
			max_view->resize(max_view_g.width, max_view_g.height/2);
		}
	}
	void retileAddedUpto3(std::vector<wayfire_view> views) {
		int size = views.size();
		if(size > 3) return;
		auto workarea = output->workspace->get_workarea();
		nothing("No. views: ",size);
		for(int i=size; i>0; i--) {
			auto v = views[i-1];
			auto vg = v->get_wm_geometry();
			nothing(v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			if(size == 1) {
				nothing("view ",i," set to ",workarea.width,"x",workarea.height," at ",0,",","0");
				v->resize(workarea.width, workarea.height);
				v->move(0, 0);
			}
			if(size == 2) {
				if(i == size) {
					nothing("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",0,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(0, 0);
				}
				else {
					nothing("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",workarea.width/2,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(workarea.width/2, 0);
				}
			}
			if(size == 3) {
				if(i == size) {
					nothing("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",0,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(0, 0);
				}
				else if(i == size-1) {
					nothing("view ",i," set to ",workarea.width/2,"x",workarea.height/2," at ",workarea.width/2,",","0");
					v->resize(workarea.width/2, workarea.height/2);
					v->move(workarea.width/2, 0);
				}
				else {
					nothing("view ",i," set to ",workarea.width/2,"x",workarea.height/2," at ",workarea.width/2,",",workarea.height/2);
					v->resize(workarea.width/2, workarea.height/2);
					v->move(workarea.width/2, workarea.height/2);
				}
			}
		}

	}

  public:
    void init() override
    {
        output->connect_signal("view-mapped", &created_cb);
        output->connect_signal("view-pre-unmapped", &removed_cb);
        output->connect_signal("view-minimized", &minimized_cb);
    }



    void fini() override
    {
        /* Destroy plugin */
        /* This time, there is nothing to destroy, since signal_connection_t
         * will disconnect automatically */
    }
};

DECLARE_WAYFIRE_PLUGIN(faketile_t)
