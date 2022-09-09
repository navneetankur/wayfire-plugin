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
		LOGD("mapped: ", view->to_string());

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
		LOGD("unmapped: ", view->to_string());
		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    this->output->workspace->get_current_workspace(), interesting_layers);
		retileRemoved(views, view);


	};

    wf::signal_connection_t workspace_changed_cb = [=] (wf::signal_data_t *data) {
        auto ev   = (wf::view_change_workspace_signal*)(data);
        auto view = get_signaled_view(data);
		if(!ev->old_workspace_valid) return;
		LOGD("ws changed: ", ev->from.x,",", ev->from.y,",", ev->to.x,",", ev->to.y);
		LOGD("current ws is ",this->output->workspace->get_current_workspace());

		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    ev->from, interesting_layers);
		retileRemoved(views,view);

		//these below lines causes the view to stay on from workspace.
		//Need to find why, or leave them commented.
		/* views = this->output->workspace->get_views_on_workspace( */
                    /* ev->to, interesting_layers); */
		/* retileAdded(views,view); */
	};

    wf::signal_connection_t minimized_cb = [=] (wf::signal_data_t *data) {
        auto ev   = (wf::view_minimized_signal*)(data);
        auto view = get_signaled_view(data);
		LOGD("minimized: ", view->to_string());
		static constexpr uint32_t interesting_layers = wf::LAYER_WORKSPACE | wf::LAYER_MINIMIZED;
		auto views = this->output->workspace->get_views_on_workspace(
                    this->output->workspace->get_current_workspace(), interesting_layers);
		if(ev->state) //minimized
			retileRemoved(views, view);
		else //restored
			retileAdded(views, view);
	};

	void retileRemoved(std::vector<wayfire_view> views, wayfire_view view) {
		auto viewg = view->get_wm_geometry();
		LOGD("retile removed: ", view->to_string());
		LOGD("it was:",view->to_string(), view->get_title(), view->get_app_id(), viewg.x,"y", viewg.y,"w", viewg.width,"h", viewg.height);
		auto temp1 = view->get_bounding_box();
		auto temp2 = view->get_output_geometry();
		LOGD("bb was:",view->to_string(), view->get_title(), view->get_app_id(), temp1.x,"y", temp1.y,"w", temp1.width,"h", temp1.height);
		LOGD("og was:",view->to_string(), view->get_title(), view->get_app_id(), temp2.x,"y", temp2.y,"w", temp2.width,"h", temp2.height);
		for(auto v: views) {
			if(v == view) continue;

			auto vg = v->get_wm_geometry();
			LOGD(v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			if(vg.height == viewg.height && vg.y == viewg.y) {
				if(vg.x + vg.width == viewg.x) {
					v->resize(vg.width + viewg.width, vg.height);
					return;
				}
				else if(viewg.x + viewg.width == vg.x) {
					v->resize(vg.width + viewg.width, vg.height);
					v->move(viewg.x, viewg.y);
					return;
				}
			}
			else if(vg.width == viewg.width && vg.x == viewg.x) {
				if(vg.y + vg.height == viewg.y) {
					v->resize(vg.width, vg.height + viewg.height);
					return;
				}
				else if(viewg.y + viewg.height == vg.y) {
					v->resize(vg.width, vg.height+viewg.height);
					v->move(viewg.x, viewg.y);
					return;
				}
			}

		}
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
		LOGD("retile addedafter3: ", view->to_string());
		int size = views.size();
		unsigned int max_area=0;
		wayfire_view max_view = nullptr;
		for(int i=0; i<size; i++) {
			auto v = views[i];
			auto vg = v->get_wm_geometry();
			LOGD(v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
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
		LOGD("No. views: ",size);
		for(int i=size; i>0; i--) {
			auto v = views[i-1];
			auto vg = v->get_wm_geometry();
			LOGD(v->to_string(), v->get_title(), v->get_app_id(), vg.x,"y", vg.y,"w", vg.width,"h", vg.height);
			if(size == 1) {
				LOGD("view ",i," set to ",workarea.width,"x",workarea.height," at ",0,",","0");
				v->resize(workarea.width, workarea.height);
				v->move(0, 0);
			}
			if(size == 2) {
				if(i == size) {
					LOGD("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",0,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(0, 0);
				}
				else {
					LOGD("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",workarea.width/2,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(workarea.width/2, 0);
				}
			}
			if(size == 3) {
				if(i == size) {
					LOGD("view ",i," set to ",workarea.width/2,"x",workarea.height," at ",0,",","0");
					v->resize(workarea.width/2, workarea.height);
					v->move(0, 0);
				}
				else if(i == size-1) {
					LOGD("view ",i," set to ",workarea.width/2,"x",workarea.height/2," at ",workarea.width/2,",","0");
					v->resize(workarea.width/2, workarea.height/2);
					v->move(workarea.width/2, 0);
				}
				else {
					LOGD("view ",i," set to ",workarea.width/2,"x",workarea.height/2," at ",workarea.width/2,",",workarea.height/2);
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
        output->connect_signal("view-change-workspace", &workspace_changed_cb);
    }



    void fini() override
    {
        /* Destroy plugin */
        /* This time, there is nothing to destroy, since signal_connection_t
         * will disconnect automatically */
    }
};

DECLARE_WAYFIRE_PLUGIN(faketile_t)
